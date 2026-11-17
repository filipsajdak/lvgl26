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
//   Binding it makes it follow its dependencies: one watcher on each of them, in their own
//   lists (lv/state.hpp), and no LVGL subject. From then on every change of a dependency
//   runs the function again and tells the bound widgets if the result is a different value
//   (a pointer result is always passed on: it cannot be compared). The last result is kept
//   for that comparison, as long as the computed follows.
//
//   raw() gives an LVGL subject, made on first use, as for a state. The subject is an
//   output. C code can read and observe it; what C code or a widget writes into it is
//   replaced at the next change and is not seen by get().
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
//   - A computed or an effect holds its watchers. reset(), and the destructor, take them
//     out of the dependencies (and delete a computed's subject): widgets bound to the
//     computed stop following, the effect stops running. Neither can be copied or moved,
//     because the dependencies point at the object.
//   - reset() of a dependency takes every watcher out of its list. What depends on it stops
//     following that dependency; get() stays correct. To follow again, reset() the
//     computed and bind it again, or reset() and start() the effect. reset() is safe in
//     any order.
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

// A watcher of a dependency, with the computed or effect it belongs to.
struct dependency_node : watch_node {
  void* owner = nullptr;
};

// The dependencies of a computed or an effect, and its watchers on them.
template <class... Deps>
class dependencies {
  static constexpr std::size_t count = sizeof...(Deps);

  std::tuple<const Deps*...> deps_;
  // Mutable: binding a computed is what makes it follow, and a binding only has a
  // pointer to const.
  mutable std::array<dependency_node, count> nodes_{};
  mutable bool following_ = false;

 public:
  constexpr explicit dependencies(const Deps&... deps) : deps_{&deps...} {}

  // f(values...), with the current value of every dependency.
  template <class F>
  decltype(auto) call(F& f) const {
    return std::apply([&f](const Deps*... d) -> decltype(auto) { return f(d->get()...); }, deps_);
  }

  bool following() const { return following_; }

  // Adds a watcher to every dependency: `changed` runs when one changes, and finds `owner`
  // in the node. Does nothing if it follows already.
  void follow(void (*changed)(watch_node&), void* owner) const {
    if (following_) return;
    following_ = true;
    std::size_t i = 0;
    std::apply([&](const Deps*... d) { (add(*d, nodes_[i++], changed, owner), ...); }, deps_);
  }

  // Takes the watchers out of the dependencies that still have them.
  void unfollow() const {
    for (auto& node : nodes_) watch_list::remove(node);
    following_ = false;
  }

 private:
  template <class D>
  static void add(const D& dep, dependency_node& node, void (*changed)(watch_node&), void* owner) {
    node.changed = changed;
    node.owner = owner;
    graph_access::watchers(dep).append(node);
  }
};

}  // namespace detail

// A value made from states and other computeds by a function. See the top of this file.
template <class F, class... Deps>
class computed : detail::watched<typename detail::result_of_call<const F&, detail::value_of<Deps>...>::type> {
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

  using base = detail::watched<value_type>;
  friend detail::graph_access;
  friend detail::source_functions<computed>;

  F f_;
  detail::dependencies<Deps...> deps_;

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

  // The LVGL subject, created on first use. The computed follows its dependencies from
  // then on. Needs lv_init() to have run. Null only if LVGL has no memory left.
  lv_subject_t* raw() const {
    follow();
    return base::subject();
  }

  // Stops following the dependencies, tells the widgets bound to it to stop following it,
  // and deletes the subject. Called by the destructor. Const, as raw() is: a `const`
  // computed can be bound, so it can be released.
  void reset() const {
    deps_.unfollow();
    base::release();
  }

 private:
  // The watchers of this computed. Asking for them makes it follow its dependencies, with
  // the result of now as the one to compare with.
  detail::watch_list& follow() const {
    if (!deps_.following()) {
      this->value_ = get();
      deps_.follow(&changed, const_cast<void*>(static_cast<const void*>(this)));
    }
    return this->watchers_;
  }

  value_type current_value() const { return base::current(); }

  static void changed(detail::watch_node& node) {
    const auto* self = static_cast<const computed*>(static_cast<detail::dependency_node&>(node).owner);
    self->publish(self->get());
  }
};

template <class F, class... D>
computed(F, D&&...) -> computed<F, typename detail::dependency_argument<D>::type...>;

// Binds a property to a computed: .hidden = lv::bind(over_limit).
template <class F, class... Deps>
constexpr binding<typename computed<F, Deps...>::value_type> bind(const computed<F, Deps...>& c) {
  return {&c, &detail::source_ops_of<computed<F, Deps...>>};
}

// Binds a text property to a computed through a format: lv::bind(gap, "{} over").
template <class F, class... Deps>
constexpr formatted_binding<typename computed<F, Deps...>::value_type> bind(
    const computed<F, Deps...>& c, text_format format) {
  return {&c, &detail::source_ops_of<computed<F, Deps...>>, format.printf};
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
  // dependency. Does nothing if the effect runs already. Always true: following a
  // state takes no memory from LVGL.
  bool start() {
    if (running_) return true;
    deps_.follow(&changed, this);
    running_ = true;
    deps_.call(f_);
    return true;
  }

  // Stops: takes the watchers out of the dependencies. start() runs the effect again.
  // Called by the destructor.
  void reset() {
    deps_.unfollow();
    running_ = false;
  }

 private:
  static void changed(detail::watch_node& node) {
    auto* self = static_cast<effect*>(static_cast<detail::dependency_node&>(node).owner);
    if (self->running_) self->deps_.call(self->f_);
  }
};

template <class F, class... D>
effect(F, D&&...) -> effect<F, typename detail::dependency_argument<D>::type...>;

}  // namespace lv
