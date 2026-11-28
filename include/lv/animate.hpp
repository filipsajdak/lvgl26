#pragma once

// Animations a coroutine can wait for.
//
//   lv::task<> open(lv::view<"arc"> arc) {
//     co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
//     co_await lv::animate(arc, {.width = {80 * lv::px, 120 * lv::px}}, 200 * lv::ms);
//   }
//
// lv::animate(widget, {.property = {from, to}}, time) runs one property of a widget
// from one value to another with an LVGL animation. The property is set to `from`
// at once and reaches `to` when the time is over. With a time of zero it is set to
// `to` at once, and there is nothing to wait for.
//
// The properties are those of lv::props that take one number: a whole number, a
// length, an angle (in degrees or in tenths) or an opacity. lv::animatable<"arc"> is the struct that lists
// them; reflection generates it from the same setters as lv::props<"arc">. Both
// ends have the property's type, so `.width = {0, 100}` does not compile.
//
// The value moves at one speed unless a fourth argument says otherwise: a name from
// lv::ease, a "cubicBezier" design token, or an lv::bezier. See lv/easing.hpp.
//
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, lv::ease::in_quad);
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, tok.motion.fall);
//
// co_await gives true when the animation ran to its end, and false when something
// stopped it before that: a newer animation of the same property of the same
// widget took over (as in LVGL, the newer one wins), or lv_anim_delete() removed
// it. A stopped wait ends in the next turn of the LVGL loop, not inside the call
// that stopped it.
//
// Like a wait for an event, a wait for an animation ties the task to the widget:
// deleting the widget ends the task. When the task is cancelled, or the animation
// loses in lv::first, the animation is deleted and the property stays where it was.

#include <lv/detail/checked.hpp>
#include <lv/easing.hpp>
#include <lv/props.hpp>
#include <lv/style.hpp>
#include <lv/task.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace lv {

namespace detail {

// What a length counts in. An animation runs between two numbers, so both ends
// must count in the same thing.
enum class length_scale { pixels, percent, negative_percent, content };

constexpr length_scale scale_of(length v) {
  const std::int32_t raw = v.raw();
  if (LV_COORD_IS_PX(raw)) return length_scale::pixels;
  if (!LV_COORD_IS_PCT(raw)) return length_scale::content;
  // LVGL stores a negative percentage in a range of its own, above the positive ones.
  return LV_COORD_GET_PCT(raw) < 0 ? length_scale::negative_percent : length_scale::percent;
}

// Whether LVGL can run an animation from `from` to `to`. One overload per type.
constexpr bool can_run_between(length from, length to) {
  return scale_of(from) == scale_of(to) && scale_of(from) != length_scale::content;
}
template <class Axis>
constexpr bool can_run_between(along<Axis> from, along<Axis> to) {
  return can_run_between(from.get(), to.get());
}
// LVGL animates 32-bit signed numbers.
template <class I>
constexpr bool can_run_between(strict<I> from, strict<I> to) {
  return std::in_range<std::int32_t>(from.raw()) && std::in_range<std::int32_t>(to.raw());
}
constexpr bool can_run_between(angle, angle) { return true; }
constexpr bool can_run_between(decidegrees, decidegrees) { return true; }
constexpr bool can_run_between(opa, opa) { return true; }
template <class U>
constexpr bool can_run_between(nonnegative<U> from, nonnegative<U> to) {
  return can_run_between(from.get(), to.get());
}

template <class T>
struct ends {
  T from{};
  T to{};
};

// The two ends, checked. A function of its own, like whole_number in lv/values.hpp:
// GCC 16.2 does not evaluate a contract on a constructor whose argument is a temporary.
template <class T>
constexpr ends<T> checked_ends(T from, T to)
  pre(can_run_between(from, to))
{
  keep_checked(can_run_between(from, to));
  return {from, to};
}

}  // namespace detail

// The two ends of an animation of one property: `.value = {0, 100}`,
// `.width = {80 * lv::px, 120 * lv::px}`. Both lengths must be in pixels, or both in
// percent of the same sign; lv::size_content cannot be animated.
template <class T>
class from_to {
  bool set_ = false;
  detail::ends<T> v_{};

 public:
  constexpr from_to() = default;

  // Accepts anything, so that a wrong value gets the message of lv::props instead
  // of the compiler's "could not convert".
  template <class U, class V>
  constexpr from_to(U&& from, V&& to)
      : set_(true),
        v_(detail::checked_ends<T>(end(std::forward<U>(from)), end(std::forward<V>(to)))) {}

  template <class U>
    requires(!std::same_as<std::remove_cvref_t<U>, from_to>)
  constexpr from_to(U&&) =
      delete("lv: an animation needs both ends: .property = {from, to}.");

  constexpr bool is_set() const { return set_; }
  constexpr T from() const { return v_.from; }
  constexpr T to() const { return v_.to; }

 private:
  template <class U>
  static constexpr T end(U&& u) {
    static_assert(std::convertible_to<U, T>, detail::wrong_value<T>::template message<U>());
    if constexpr (std::convertible_to<U, T>)
      return static_cast<T>(std::forward<U>(u));
    else
      return T{};  // unreachable: keeps the message above the only error
  }
};

namespace detail {

// Whether a property of this type can be animated: a number that LVGL can count
// through. Not bool, not a time, not an enum, not text.
consteval bool is_animatable_type(std::meta::info t) {
  for (auto unit : {(^^opa), (^^angle), (^^decidegrees), (^^length), std::meta::dealias(^^horizontal),
                    std::meta::dealias(^^vertical)})
    if (t == unit) return true;
  if (is_made_from(t, ^^nonnegative)) return is_animatable_type(std::meta::template_arguments_of(t)[0]);
  return is_made_from(t, ^^strict) && std::meta::template_arguments_of(t)[0] != ^^bool;
}

// Whether this member of props can be animated: a setter that takes one such value.
consteval bool is_animatable(const member_item& it) {
  if (it.target == std::meta::info{} || !std::meta::is_function(it.target)) return false;
  if (std::meta::parameters_of(it.target).size() != 2) return false;
  return is_animatable_type(std::meta::template_arguments_of(it.type)[0]);
}

// One property that can be animated: the type of its two ends, the function that sets it, and whether
// that is a style setter (it takes the selector as a third argument) or a setter of the widget.
struct animatable_item {
  std::string_view name;
  std::meta::info value;
  std::meta::info setter;
  bool style = false;
};

// The setters of the widget that take one number, in the order of lv::props<widget>.
// D only delays the scan until an animation is really written: see props_of.
template <class D = void>
consteval std::vector<animatable_item> widget_animatables(std::string_view widget) {
  std::vector<animatable_item> out;
  for (const auto& it : members<D>(widget))
    if (is_animatable(it)) out.push_back({it.name, std::meta::template_arguments_of(it.type)[0], it.target, false});
  return out;
}

// The style properties that take one number: `lv_obj_set_style_<name>(obj, value, selector)` with a
// value of a unit that an animation can count through, in the order of lv::style.
template <class D = void>
consteval std::vector<animatable_item> style_animatables() {
  std::vector<animatable_item> out;
  for (std::size_t i = 0; i < style_table<D>.size(); i += 3) {
    const auto type = style_table<D>[i];
    const auto setter = style_table<D>[i + 1];
    if (std::meta::parameters_of(setter).size() != 3 || !is_made_from(type, ^^arg)) continue;
    const auto args = std::meta::template_arguments_of(type);
    if (args.size() != 1 || !is_made_from(args[0], ^^styled)) continue;
    const auto value = std::meta::template_arguments_of(args[0])[0];
    if (is_animatable_type(value))
      out.push_back({std::meta::identifier_of(setter).substr(style_prefix.size()), value, setter, true});
  }
  return out;
}

// What animatable<W> holds: the setters of the widget, then the style properties that no setter of
// the widget has the name of (`.width` is the widget's, whichever way it is written). The width
// "" is the struct of lv::in: the style properties alone, a name that is both included.
template <class D = void>
consteval std::vector<animatable_item> animatable_items(std::string_view widget) {
  std::vector<animatable_item> out;
  if (!widget.empty()) out = widget_animatables<D>(widget);
  const std::size_t widget_items = out.size();
  for (const auto& item : style_animatables<D>()) {
    bool taken = false;
    for (std::size_t i = 0; i < widget_items && !taken; ++i) taken = out[i].name == item.name;
    if (!taken) out.push_back(item);
  }
  return out;
}

template <class D = void>
consteval std::vector<std::meta::info> animatable_specs(std::string_view widget) {
  std::vector<std::meta::info> specs;
  for (const auto& it : animatable_items<D>(widget))
    specs.push_back(std::meta::data_member_spec(std::meta::substitute(^^from_to, {it.value}), {.name = str(it.name)}));
  return specs;
}

template <class D = void>
consteval std::vector<std::meta::info> animatable_setters(std::string_view widget) {
  std::vector<std::meta::info> out;
  for (const auto& it : animatable_items<D>(widget)) out.push_back(it.setter);
  return out;
}

// For each member, in order: 1 when its setter is a style setter.
template <class D = void>
consteval std::vector<int> animatable_styles(std::string_view widget) {
  std::vector<int> out;
  for (const auto& it : animatable_items<D>(widget)) out.push_back(it.style ? 1 : 0);
  return out;
}

// The second parameter lets a signature name the struct lazily, as for props_of.
template <fixed W, class = void>
struct animatable_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, animatable_specs<>(W.sv())); }
};

template <fixed W, class D>
using lazy_animatable = typename animatable_of<W, D>::type;

}  // namespace detail

// The properties of a widget that can be animated, each with its two ends.
// See the top of this file.
template <detail::fixed W>
using animatable = typename detail::animatable_of<W>::type;

// The style properties that take one number, as lv::in takes them: the struct of lv::animate
// for a part and a state. A name that is also a setter of a widget is a member here. An alias
// template, written lv::animatable_style<>: a plain alias would generate the struct where this
// header is read, in every file (tests/include_only.cpp).
template <class D = void>
using animatable_style = detail::lazy_animatable<"", D>;

namespace detail {

// The setter behind each member of animatable<W>, in member order.
template <fixed W>
inline constexpr std::span<const std::meta::info> setters_of_animatable =
    std::define_static_array(animatable_setters<>(W.sv()));

// For each member of animatable<W>: 1 when it is a style property.
template <fixed W>
inline constexpr std::span<const int> styles_of_animatable = std::define_static_array(animatable_styles<>(W.sv()));

template <fixed W>
inline constexpr auto members_of_animatable = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^animatable<W>, std::meta::access_context::unchecked()));

// Between LVGL's animation value and the type a setter or a unit holds. The same
// type is passed on as it is: a cast to itself is a warning.
template <class To, class From>
constexpr To number_as(From v) {
  if constexpr (std::same_as<To, From>)
    return v;
  else
    return static_cast<To>(v);
}

// What LVGL calls for every step of an animation: the property's setter.
template <std::meta::info Setter>
void animation_step(void* widget, std::int32_t value) {
  using parameter = typename[:std::meta::type_of(std::meta::parameters_of(Setter)[1]):];
  [:Setter:](static_cast<lv_obj_t*>(widget), number_as<parameter>(value));
}

// The same for a style property of one part and state: the selector is part of what LVGL
// calls the animation, so it is an argument of the template, and two selectors are two animations.
template <std::meta::info Setter, lv_style_selector_t Selector>
void style_step(void* widget, std::int32_t value) {
  using parameter = typename[:std::meta::type_of(std::meta::parameters_of(Setter)[1]):];
  [:Setter:](static_cast<lv_obj_t*>(widget), number_as<parameter>(value), Selector);
}

// One running LVGL animation of one property, as something to wait for.
//
// LVGL identifies an animation by the widget and the step function, and lets only
// one of them run: starting a second one deletes the first. So at most one source
// owns the animation of a property at any time, and disarm() can delete it by
// those two.
struct animation_source {
  lv_obj_t* widget = nullptr;
  lv_anim_exec_xcb_t step = nullptr;
  std::int32_t from = 0;
  std::int32_t to = 0;
  std::uint32_t ms = 0;
  easing how{};  // how the value moves between its ends; the default is one speed
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_anim_t* running = nullptr;  // the animation, while it runs
  lv_timer_t* stopped = nullptr; // set after the animation was stopped: ends the wait
  bool finished = false;

  animation_source(lv_obj_t* w, std::uint32_t time) : widget(w), ms(time) {}
  animation_source(animation_source&&) = default;  // only moved before it is armed
  ~animation_source() { disarm(); }

  // Not noexcept: a function with a precondition should let a violation handler throw.
  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base* owner)
    pre(widget != nullptr)  // the view is not empty
    pre(step != nullptr)    // a property was given
    pre(owner != nullptr)
  {
    done = done_fn;
    context = done_context;
    // As for an event: without the record that ties the task to the widget, the
    // task ends here instead of waiting unprotected.
    if (watch(owner, widget) == nullptr) {
      cancel(owner);
      return true;  // stay suspended, with nothing armed, until the task is ended
    }
    // LVGL never reaches the end value of an animation that takes no time.
    if (ms != 0) running = start();
    if (running != nullptr) return true;
    // No time, or no memory for an animation: the property gets its final value at
    // once, without a wait. An older animation of it is over, as if this one had run.
    lv_anim_delete(widget, step);
    step(widget, to);
    finished = true;
    return false;
  }

  void disarm() noexcept {
    if (running != nullptr) {
      lv_anim_set_deleted_cb(running, nullptr);  // this deletion is ours: no wait to end
      running = nullptr;
      lv_anim_delete(widget, step);
    }
    if (stopped != nullptr) lv_timer_delete(stopped);
    stopped = nullptr;
  }

  // What the wait gives: whether the animation ran to its end.
  bool value() const noexcept { return finished; }

  // For lv::first: what to do when the animation ends first. The handler may take
  // whether it ran to its end.
  template <class F>
    requires std::invocable<F&, bool> || std::invocable<F&>
  handled<animation_source, F> then(F handler) && {
    return {std::move(*this), std::move(handler)};
  }

  // The animation ran to its end. LVGL has taken it out of its list already and
  // frees it when this returns.
  static void completed_cb(lv_anim_t* a) {
    auto* self = static_cast<animation_source*>(lv_anim_get_user_data(a));
    lv_anim_set_deleted_cb(a, nullptr);  // LVGL would call it next, after `self` may be gone
    self->running = nullptr;
    self->finished = true;
    self->done(self->context);  // may destroy this source: nothing after this line
  }

  // Something else deleted the animation: a newer one of the same property, or
  // lv_anim_delete(). The task must not run inside that call (LVGL is in the middle
  // of changing its list of animations), so the wait ends from a timer.
  static void deleted_cb(lv_anim_t* a) {
    auto* self = static_cast<animation_source*>(lv_anim_get_user_data(a));
    self->running = nullptr;
    self->stopped = lv_timer_create(&stopped_cb, 0, self);
    // Without memory for the timer the task stays suspended until its widget is deleted.
    if (self->stopped != nullptr) lv_timer_set_repeat_count(self->stopped, 1);
  }

  static void stopped_cb(lv_timer_t* t) {
    auto* self = static_cast<animation_source*>(lv_timer_get_user_data(t));
    self->stopped = nullptr;  // LVGL deletes it when this callback returns
    self->done(self->context);
  }

 private:
  lv_anim_t* start() {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, widget);
    lv_anim_set_exec_cb(&a, step);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, ms);
    how.apply(&a);
    lv_anim_set_completed_cb(&a, &completed_cb);
    lv_anim_set_deleted_cb(&a, &deleted_cb);
    lv_anim_set_user_data(&a, this);
    return lv_anim_start(&a);
  }
};

// How many properties of `what` are set.
template <fixed W>
constexpr std::size_t count_set(const animatable<W>& what) {
  std::size_t n = 0;
  template for (constexpr auto m : members_of_animatable<W>) {
    if (what.[:m:].is_set()) ++n;
  }
  return n;
}

// The animation of the property that is set in `what`. A style property is animated in `Selector`
// (the main part in the default state, unless lv::in says another).
template <fixed W, lv_style_selector_t Selector = 0>
animation_source animation_of(lv_obj_t* widget, const animatable<W>& what, duration time,
                              const easing& how) {
  animation_source source{widget, time.ms};
  source.how = how;
  template for (constexpr auto i : indices<members_of_animatable<W>.size()>) {
    const auto& ends = what.[:members_of_animatable<W>[i]:];
    if (ends.is_set()) {
      if constexpr (styles_of_animatable<W>[i] != 0)
        source.step = &style_step<setters_of_animatable<W>[i], Selector>;
      else
        source.step = &animation_step<setters_of_animatable<W>[i]>;
      source.from = number_as<std::int32_t>(lower(ends.from()));
      source.to = number_as<std::int32_t>(lower(ends.to()));
    }
  }
  return source;
}

}  // namespace detail

// bool finished = co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
// Runs one property of a widget from one value to another, at one speed or along an
// easing curve. See the top of this file.
template <detail::fixed W, class D = void>
detail::animation_source animate(view<W> widget, const detail::lazy_animatable<W, D>& what,
                                 duration time, const easing& how = {})
  pre(widget.raw() != nullptr)
  pre(detail::count_set<W>(what) == 1)             // one property for each animation
  pre(std::in_range<std::int32_t>(time.ms))        // LVGL counts an animation's time in 31 bits
  pre(detail::usable(how.curve()))                 // a curve LVGL can follow: see lv/easing.hpp
{
  return detail::animation_of<W>(widget.raw(), what, time, how);
}

// A widget and the part and state whose local style values an animation moves: lv::in<"knob">(view).
template <detail::fixed W, lv_style_selector_t Selector>
struct view_in {
  view<W> widget;
};

namespace detail {

// The selector of lv::in<Names...>: one name is a part or a state, two are the part and then the state.
// The messages are those of lv::on, for lv::in.
template <fixed First, fixed... Rest>
consteval lv_style_selector_t selector_of_names() {
  static_assert(sizeof...(Rest) <= 1,
                "lv: lv::in takes a part and a state at most: lv::in<\"knob\", \"pressed\">(view).");
  if constexpr (sizeof...(Rest) == 0) {
    return part_or_state_named<>(First.sv());
  } else {
    constexpr std::string_view second = (Rest.sv(), ...);
    if (!selector_named<lv_part_t>(First.sv()) && selector_named<lv_state_t>(First.sv()))
      fail("lv: '" + str(First.sv()) + "' is a state. lv::in takes the part first, then the state: lv::in<\"knob\", \"" +
           str(First.sv()) + "\">(view).");
    if (!selector_named<lv_state_t>(second) && selector_named<lv_part_t>(second))
      fail("lv: '" + str(second) + "' is a part. lv::in takes the part first, then the state: lv::in<\"" + str(second) +
           "\", \"pressed\">(view).");
    if (!selector_named<lv_part_t>(First.sv()))
      fail("lv: widgets have no part '" + str(First.sv()) + "'. The parts: " + selector_names<lv_part_t>() + ".");
    if (!selector_named<lv_state_t>(second))
      fail("lv: widgets have no state '" + str(second) + "'. The states: " + selector_names<lv_state_t>() + ".");
    return *selector_named<lv_part_t>(First.sv()) | *selector_named<lv_state_t>(second);
  }
}

}  // namespace detail

// The local style values of a part and a state, as what lv::animate moves: the properties of lv::style
// that take one number, whatever the widget (`.width` is the style value here, not the widget's).
//
//   co_await lv::animate(lv::in<"indicator">(arc), {.arc_width = {2 * lv::px, 8 * lv::px}}, 120 * lv::ms);
//   co_await lv::animate(lv::in<"knob", "pressed">(slider), {.opa = {0, 255}}, 200 * lv::ms);
template <detail::fixed First, detail::fixed... Rest, detail::fixed W>
constexpr view_in<W, detail::selector_of_names<First, Rest...>()> in(view<W> widget) {
  return {widget};
}

template <detail::fixed W, lv_style_selector_t Selector, class D = void>
detail::animation_source animate(view_in<W, Selector> target, const detail::lazy_animatable<"", D>& what,
                                 duration time, const easing& how = {})
  pre(target.widget.raw() != nullptr)
  pre(detail::count_set<"">(what) == 1)
  pre(std::in_range<std::int32_t>(time.ms))
  pre(detail::usable(how.curve()))
{
  return detail::animation_of<"", Selector>(target.widget.raw(), what, time, how);
}

}  // namespace lv
