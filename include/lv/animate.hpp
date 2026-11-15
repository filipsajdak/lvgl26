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
// length, an angle or an opacity. lv::animatable<"arc"> is the struct that lists
// them; reflection generates it from the same setters as lv::props<"arc">. Both
// ends have the property's type, so `.width = {0, 100}` does not compile.
//
// The value moves at one speed unless a fourth argument gives an easing curve: an
// lv::bezier, the two control points of a cubic Bezier curve as in CSS and in a
// "cubicBezier" design token.
//
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, *tok.motion.fall);
//   co_await lv::animate(ball, {.y = {floor, top}}, 400 * lv::ms, lv::bezier{0.33, 0.67, 0.67, 1});
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
#include <lv/props.hpp>
#include <lv/task.hpp>
#include <lv/token_types.hpp>
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
constexpr bool can_run_between(opa, opa) { return true; }

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
  for (auto unit : {(^^opa), (^^angle), (^^length), std::meta::dealias(^^horizontal),
                    std::meta::dealias(^^vertical)})
    if (t == unit) return true;
  return is_made_from(t, ^^strict) && std::meta::template_arguments_of(t)[0] != ^^bool;
}

// Whether this member of props can be animated: a setter that takes one such value.
consteval bool is_animatable(const member_item& it) {
  if (it.target == std::meta::info{} || !std::meta::is_function(it.target)) return false;
  if (std::meta::parameters_of(it.target).size() != 2) return false;
  return is_animatable_type(std::meta::template_arguments_of(it.type)[0]);
}

// The members of props<widget> that can be animated, in the same order.
// D only delays the scan until an animation is really written: see props_of.
template <class D = void>
consteval std::vector<member_item> animatable_members(std::string_view widget) {
  std::vector<member_item> out;
  for (const auto& it : members<D>(widget))
    if (is_animatable(it)) out.push_back(it);
  return out;
}

template <class D = void>
consteval std::vector<std::meta::info> animatable_specs(std::string_view widget) {
  std::vector<std::meta::info> specs;
  for (const auto& it : animatable_members<D>(widget)) {
    const auto type =
        std::meta::substitute(^^from_to, {std::meta::template_arguments_of(it.type)[0]});
    specs.push_back(std::meta::data_member_spec(type, {.name = str(it.name)}));
  }
  return specs;
}

template <class D = void>
consteval std::vector<std::meta::info> animatable_setters(std::string_view widget) {
  std::vector<std::meta::info> out;
  for (const auto& it : animatable_members<D>(widget)) out.push_back(it.target);
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

namespace detail {

// The setter behind each member of animatable<W>, in member order.
template <fixed W>
inline constexpr std::span<const std::meta::info> setters_of_animatable =
    std::define_static_array(animatable_setters<>(W.sv()));

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

// One running LVGL animation of one property, as something to wait for.
//
// LVGL identifies an animation by the widget and the step function, and lets only
// one of them run: starting a second one deletes the first. So at most one source
// owns the animation of a property at any time, and disarm() can delete it by
// those two.
// A control point of an easing curve as LVGL counts it: 0 to 1 is 0 to 1024.
constexpr std::int16_t bezier_part(double v) {
  return static_cast<std::int16_t>(v * 1024 + (v < 0 ? -0.5 : 0.5));
}

// Whether `v` is from `low` to `high`.
constexpr bool within(double v, double low, double high) { return v >= low && v <= high; }

// Whether LVGL can follow this curve: time only moves forward, so both x are from 0
// to 1, as in CSS; a y may go beyond its ends, for a curve that overshoots.
constexpr bool usable(const bezier& curve) {
  return within(curve.x1, 0, 1) && within(curve.x2, 0, 1) && within(curve.y1, -8, 8) &&
         within(curve.y2, -8, 8);
}

struct animation_source {
  lv_obj_t* widget = nullptr;
  lv_anim_exec_xcb_t step = nullptr;
  std::int32_t from = 0;
  std::int32_t to = 0;
  std::uint32_t ms = 0;
  bezier curve{};  // how the value moves between its ends; the default is a straight line
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
    if (watch(owner, widget, true) == nullptr) {
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
    if (curve != bezier{}) {
      lv_anim_set_path_cb(&a, &lv_anim_path_custom_bezier3);
      lv_anim_set_bezier3_param(&a, bezier_part(curve.x1), bezier_part(curve.y1), bezier_part(curve.x2),
                                bezier_part(curve.y2));
    }
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

// The animation of the property that is set in `what`.
template <fixed W>
animation_source animation_of(lv_obj_t* widget, const animatable<W>& what, duration time,
                              const bezier& curve) {
  animation_source source{widget, time.ms};
  source.curve = curve;
  template for (constexpr auto i : indices<members_of_animatable<W>.size()>) {
    const auto& ends = what.[:members_of_animatable<W>[i]:];
    if (ends.is_set()) {
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
                                 duration time, const bezier& curve = {})
  pre(widget.raw() != nullptr)
  pre(detail::count_set<W>(what) == 1)             // one property for each animation
  pre(std::in_range<std::int32_t>(time.ms))        // LVGL counts an animation's time in 31 bits
  pre(detail::usable(curve))                       // both x of the curve are from 0 to 1
{
  return detail::animation_of<W>(widget.raw(), what, time, curve);
}

}  // namespace lv
