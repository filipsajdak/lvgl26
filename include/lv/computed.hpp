#pragma once

// Derived state and effects: values and actions that follow lv::state objects.
//
//   constinit lv::state speed{0 * km / h};
//   constinit lv::state limit{50 * km / h};
//   using speed_t = decltype(speed.get());             // the quantity type; `auto` works too
//
//   // A value made from other values. It has no storage of its own.
//   constinit lv::computed over_limit{[](speed_t s, speed_t l) { return s > l; }, speed, limit};
//
//   over_limit.get()                                   // false
//   lv::label({.hidden = lv::bind(over_limit), .text = "Within the limit"})   // like a state
//
//   // An action that runs now and after every change.
//   constinit lv::effect log{[](speed_t s) { std::printf("%d\n", s.numerical_value_in(km / h)); },
//                            speed};
//   log.start();
//
// The dependencies are written once, where the object is defined. Their types are
// part of its type and their addresses are constants in it, so a computed or an
// effect can be a constant-initialized global (`constinit`, as for a state; not
// `constexpr`), nothing is registered at run time, and a cycle cannot be written:
// an object can only depend on what exists before it.
//
// The function takes the dependencies' values in their order. The result of a
// computed follows the rules of a state: a quantity, a named value, a bool, an enum,
// a color or a pointer, and no plain integer.
//
// How a computed is evaluated
//
//   get() calls the function with the current values of the dependencies, every
//   time. Nothing is cached, so get() is never out of date, and a computed that
//   nothing is bound to costs no memory and no work. Keep the function cheap and
//   free of side effects.
//
//   Binding it (or asking for raw()) creates one lv_subject_t, as for a state, and
//   one LVGL observer on the subject of each dependency. From then on every change
//   of a dependency runs the function again and writes the result to the subject.
//   LVGL tells the bound widgets only if the result is a different value (a pointer
//   result is always passed on: LVGL cannot compare what it points at).
//
//   The subject is an output. C code can read and observe it; what C code or a
//   widget writes into it is replaced at the next change and is not seen by get().
//
// States that change together
//
//   There is no transaction. `speed = a; limit = b;` is two changes, and after the
//   first one a computed of both has the value for the new speed and the old limit.
//   Because a computed reads its dependencies through get(), a value built on two
//   computeds of the same state (a diamond) is never made from one new and one old
//   input: when the state changes, the first notification already gives the final
//   value. The second one finds the same value, so bound widgets are not told twice.
//   An effect has no value to compare: it runs once for each dependency that changed,
//   in a diamond twice, both times with the final values.
//
// Lifetime
//
//   - A dependency must outlive what depends on it. A temporary is refused.
//   - A computed or an effect owns its observers. reset(), and the destructor,
//     delete them (and a computed's subject): widgets bound to the computed stop
//     following, the effect stops running. Neither can be copied or moved, because
//     the observers point at the object.
//   - reset() of a dependency deletes its subject, and LVGL deletes the observers on
//     it. What depends on it stops following that dependency; get() stays correct.
//     To follow again, reset() the computed and bind it again, or reset() and
//     start() the effect. reset() is safe in any order.
//   - Like a state, release everything with reset() before lv_deinit().
//
// Like all of LVGL, use these from the UI thread only.

#include <lv/state.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <meta>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace lv {

template <class F, class... Deps>
class computed;

namespace detail {

// The value type of a dependency: of a state, of a computed, or this marker for
// anything else.
struct not_a_dependency {};

template <class D>
struct dependency_value {
  using type = not_a_dependency;
};
template <class T, class O>
struct dependency_value<state<T, O>> {
  using type = T;
};
template <class F, class... Deps>
struct dependency_value<computed<F, Deps...>> {
  using type = typename computed<F, Deps...>::value_type;
};

template <class D>
using value_of = typename dependency_value<D>::type;

}  // namespace detail

// An lv::state or an lv::computed: what a widget can follow and a computed can depend on.
// Where a part takes a state, as `const lv::dependency auto& count` or as the template
// argument `template <lv::dependency auto& Count>`.
template <class D>
concept dependency = !std::same_as<detail::value_of<D>, detail::not_a_dependency>;

namespace detail {

// What class template argument deduction makes of a dependency that was passed as
// a temporary: not a dependency, so that it is refused.
template <class D>
struct temporary {};

template <class D>
struct dependency_argument {
  using type = temporary<D>;
};
template <class D>
struct dependency_argument<D&> {
  using type = std::remove_const_t<D>;
};

// What F returns for these values. If it cannot be called with them, a type that
// passes every later check, so that only the first mistake is reported.
template <class F, class... Values>
struct result_of_call {
  using type = bool;
};
template <class F, class... Values>
  requires std::invocable<F, Values...>
struct result_of_call<F, Values...> {
  using type = std::remove_cvref_t<std::invoke_result_t<F, Values...>>;
};

// The settings tag, named through a template parameter so that lv::cfg is read
// when the computed is used and not when this header is.
template <class>
struct settings {
  using tag = override;
};

consteval std::string_view bad_dependency(std::string_view what) {
  return std::define_static_string(
      "lv: a dependency of an " + str(what) + " must be an lv::state or an lv::computed that "
      "outlives it. Pass the object itself, as in `" + str(what) + "{f, speed}`: not its value "
      "(`speed.get()`) and not a temporary.");
}

template <class... Deps>
consteval std::string_view not_callable(std::string_view what, std::string_view how) {
  std::string m = "lv: the function of an " + str(what) +
                  " takes the values of its dependencies, in their order: (";
  std::string sep;
  ((m += sep + str(std::meta::display_string_of(std::meta::dealias(^^value_of<Deps>))), sep = ", "),
   ...);
  return std::define_static_string(m + "). Take each by value or by const reference." + str(how));
}

template <class R>
consteval std::string_view bad_result() {
  return std::define_static_string(
      "lv: the function of an lv::computed returns " +
      str(std::meta::display_string_of(std::meta::dealias(^^R))) +
      ", which a state cannot hold. Return a quantity with an integer number (`s - l`, with "
      "mp-units), a struct with one integer, bool or enum member, a bool, an enum, an "
      "lv_color_t or a pointer (for text: const char*). For a side effect, use lv::effect.");
}

// The LVGL subject behind a computed, created on first use.
template <class C>
lv_subject_t* raw_of(const void* source) {
  return static_cast<const C*>(source)->raw();
}

// The dependencies of a computed or an effect, and its observers on them.
template <class... Deps>
class dependencies {
  static constexpr std::size_t count = sizeof...(Deps);

  std::tuple<const Deps*...> deps_;
  // Mutable: binding a computed is what subscribes it, and a binding only has a
  // pointer to const.
  mutable std::array<lv_observer_t*, count> observers_{};
  mutable std::array<std::uint32_t, count> epochs_{};

 public:
  constexpr explicit dependencies(const Deps&... deps) : deps_{&deps...} {}

  // f(values...), with the current value of every dependency.
  template <class F>
  decltype(auto) call(F& f) const {
    return std::apply([&f](const Deps*... d) -> decltype(auto) { return f(d->get()...); }, deps_);
  }

  // Adds an observer to every dependency, or to none: false if LVGL has no memory.
  // LVGL calls a new observer once at once; the callback must ignore those calls.
  bool subscribe(lv_observer_cb_t changed, void* owner) const {
    std::size_t i = 0;
    const bool ok = std::apply(
        [&](const Deps*... d) { return (follow(*d, i++, changed, owner) && ...); }, deps_);
    if (!ok) unsubscribe();
    return ok;
  }

  // Deletes the observers that still exist.
  void unsubscribe() const {
    const bool lvgl_is_up = lv_is_initialized();
    std::size_t i = 0;
    std::apply([&](const Deps*... d) { (unfollow(*d, i++, lvgl_is_up), ...); }, deps_);
  }

 private:
  template <class D>
  bool follow(const D& dep, std::size_t i, lv_observer_cb_t changed, void* owner) const {
    lv_subject_t* subject = dep.raw();
    if (subject == nullptr) return false;
    observers_[i] = lv_subject_add_observer(subject, changed, owner);
    epochs_[i] = graph_access::epoch(dep);
    return observers_[i] != nullptr;
  }

  // The observer is gone already if the dependency deleted its subject since
  // follow(), or if LVGL was shut down.
  template <class D>
  void unfollow(const D& dep, std::size_t i, bool lvgl_is_up) const {
    const bool exists = observers_[i] != nullptr && epochs_[i] == graph_access::epoch(dep);
    if (exists && lvgl_is_up) lv_observer_delete(observers_[i]);
    observers_[i] = nullptr;
  }
};

}  // namespace detail

// A value made from states and other computeds by a function. See the top of this file.
template <class F, class... Deps>
class computed {
  static constexpr bool dependencies_ok = (dependency<Deps> && ...);
  static_assert(dependencies_ok, detail::bad_dependency("lv::computed"));
  static_assert(!dependencies_ok || std::invocable<const F&, detail::value_of<Deps>...>,
                detail::not_callable<Deps...>(
                    "lv::computed", " It is called as const: a `mutable` lambda does not fit."));

 public:
  using value_type =
      typename detail::result_of_call<const F&, detail::value_of<Deps>...>::type;

 private:
  static_assert(state_value<value_type>, detail::bad_result<value_type>());
  static_assert(!detail::integer<value_type> ||
                    cfg<typename detail::settings<F>::tag>.raw_integer_state,
                "lv: the function of an lv::computed returns a plain integer, which does not "
                "say what the number is. Return a quantity (the difference of two speeds is "
                "one), an lv::counter, or a named type: `struct tab_index { int value; };`. To "
                "allow plain integers, set lv::config::raw_integer_state.");

  F f_;
  detail::dependencies<Deps...> deps_;
  mutable lv_subject_t* subject_ = nullptr;
  mutable std::uint32_t epoch_ = 0;  // see detail::graph_access
  friend detail::graph_access;

 public:
  constexpr computed(F f, const Deps&... deps) : f_(std::move(f)), deps_(deps...) {}
  computed(const computed&) = delete;
  computed& operator=(const computed&) = delete;
  // Constexpr only to refuse a `constexpr` computed with a message: GCC 16.2 puts
  // such a variable in read-only memory although it has mutable members, and
  // binding it would write there.
  constexpr ~computed() {
    if consteval {
      detail::fail(
          "lv: an lv::computed cannot be a constexpr variable: binding it writes to it. "
          "Write `constinit lv::computed`, as for a state.");
    } else {
      reset();
    }
  }

  // The current value: the function of the dependencies' current values.
  value_type get() const { return deps_.call(f_); }

  // The LVGL subject, created on first use, with an observer on every dependency.
  // Needs lv_init() to have run. Null only if LVGL has no memory left.
  lv_subject_t* raw() const {
    if (subject_ == nullptr) subject_ = make_subject();
    return subject_;
  }

  // Deletes the subject and the observers. Widgets bound to it stop following.
  // Called by the destructor. Const, as raw() is: a `const` computed can be bound,
  // so it can be released.
  void reset() const {
    if (subject_ == nullptr) return;
    deps_.unsubscribe();
    if (lv_is_initialized()) lv_subject_delete(subject_);
    ++epoch_;
    subject_ = nullptr;
  }

 private:
  lv_subject_t* make_subject() const {
    lv_subject_t* s = lv_subject_create(detail::subject_type<value_type>());
    if (s == nullptr) return nullptr;
    // subject_ is still null here, so changed() ignores LVGL's first calls.
    if (!deps_.subscribe(&changed, const_cast<void*>(static_cast<const void*>(this)))) {
      lv_subject_delete(s);
      return nullptr;
    }
    detail::write_subject(s, get());
    return s;
  }

  static void changed(lv_observer_t* observer, lv_subject_t*) {
    const auto* self = static_cast<const computed*>(lv_observer_get_user_data(observer));
    if (self->subject_ != nullptr) detail::write_subject(self->subject_, self->get());
  }
};

template <class F, class... D>
computed(F, D&&...) -> computed<F, typename detail::dependency_argument<D>::type...>;

// Binds a property to a computed: .hidden = lv::bind(over_limit).
template <class F, class... Deps>
constexpr binding<typename computed<F, Deps...>::value_type> bind(const computed<F, Deps...>& c) {
  return {&c, &detail::raw_of<computed<F, Deps...>>};
}

// Binds a text property to a computed through a format: lv::bind(gap, "{} over").
template <class F, class... Deps>
constexpr formatted_binding<typename computed<F, Deps...>::value_type> bind(
    const computed<F, Deps...>& c, text_format format) {
  return {&c, &detail::raw_of<computed<F, Deps...>>, format.printf};
}

// A function that runs when states or computeds change. See the top of this file.
// The function is kept in the effect, so it may capture anything.
template <class F, class... Deps>
class effect {
  static constexpr bool dependencies_ok = (dependency<Deps> && ...);
  static_assert(dependencies_ok, detail::bad_dependency("lv::effect"));
  static_assert(!dependencies_ok || std::invocable<F&, detail::value_of<Deps>...>,
                detail::not_callable<Deps...>("lv::effect", ""));

  F f_;
  detail::dependencies<Deps...> deps_;
  bool running_ = false;

 public:
  constexpr effect(F f, const Deps&... deps) : f_(std::move(f)), deps_(deps...) {}
  effect(const effect&) = delete;
  effect& operator=(const effect&) = delete;
  ~effect() { reset(); }

  // Runs the function once, now, and from then on after every change of a
  // dependency. Does nothing if the effect runs already. Needs lv_init() to have
  // run. False only if LVGL has no memory left: then nothing is observed.
  bool start() {
    if (running_) return true;
    // running_ is still false here, so changed() ignores LVGL's first calls.
    if (!deps_.subscribe(&changed, this)) return false;
    running_ = true;
    deps_.call(f_);
    return true;
  }

  // Stops: deletes the observers. start() runs the effect again.
  // Called by the destructor.
  void reset() {
    deps_.unsubscribe();
    running_ = false;
  }

 private:
  static void changed(lv_observer_t* observer, lv_subject_t*) {
    auto* self = static_cast<effect*>(lv_observer_get_user_data(observer));
    if (self->running_) self->deps_.call(self->f_);
  }
};

template <class F, class... D>
effect(F, D&&...) -> effect<F, typename detail::dependency_argument<D>::type...>;

}  // namespace lv
