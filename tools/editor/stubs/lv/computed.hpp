#pragma once

// Editor stub of lv/computed.hpp: values and actions that follow states.
//
//   constinit lv::computed over_limit{[](auto s, auto l) { return s > l; }, speed, limit};
//   constinit lv::effect log{[](auto s) { ... }, speed};

#include <lv/state.hpp>

#include <type_traits>

namespace lv {

template <class F, class... Deps>
class computed;

namespace detail {

struct not_a_dependency {};

template <class D>
struct value_of_dependency {
  using type = not_a_dependency;
};
template <class T, class O>
struct value_of_dependency<state<T, O>> {
  using type = T;
};
template <class F, class... Deps>
struct value_of_dependency<computed<F, Deps...>> {
  using type = typename computed<F, Deps...>::value_type;
};

/// What a dependency gives the function: the value of a state or of a computed.
template <class D>
using value_of = typename value_of_dependency<D>::type;

}  // namespace detail

/// An lv::state or an lv::computed: what a widget can follow and a computed can depend on.
/// Where a part takes a state, as `const lv::dependency auto& count` or as the template
/// argument `template <lv::dependency auto& Count>`.
template <class D>
concept dependency = !std::same_as<detail::value_of<D>, detail::not_a_dependency>;

/// A value made from states and other computed values. Nothing is stored: get()
/// calls the function. Bind it like a state. Write `constinit lv::computed`.
template <class F, class... Deps>
class computed {
 public:
  /// What the function returns.
  using value_type = std::invoke_result_t<const F&, detail::value_of<Deps>...>;

  constexpr computed(F, const Deps&...) {}
  computed(const computed&) = delete;
  computed& operator=(const computed&) = delete;

  /// Calls the function with the current values.
  value_type get() const { __builtin_unreachable(); }
  /// The LVGL subject, created on first use.
  lv_subject_t* raw() const { __builtin_unreachable(); }
  /// Stops following the dependencies and deletes the subject. Widgets bound to it stop following.
  void reset() const {}
};

template <class F, class... D>
computed(F, D&&...) -> computed<F, std::remove_cvref_t<D>...>;

/// Binds a property to a computed value: .hidden = lv::bind(over_limit).
template <class F, class... Deps>
constexpr binding<typename computed<F, Deps...>::value_type> bind(const computed<F, Deps...>&) {
  return {};
}

/// Binds a text property to a computed value through a format.
template <class F, class... Deps>
constexpr formatted_binding<typename computed<F, Deps...>::value_type> bind(const computed<F, Deps...>&,
                                                                           text_format) {
  return {};
}

/// An action that follows states: its function runs at start() and after every change.
template <class F, class... Deps>
class effect {
 public:
  constexpr effect(F, const Deps&...) {}
  effect(const effect&) = delete;
  effect& operator=(const effect&) = delete;
  ~effect() {}

  /// Runs the function now, and after every change. Always true.
  bool start() { __builtin_unreachable(); }
  /// Stops following.
  void reset() {}
};

template <class F, class... D>
effect(F, D&&...) -> effect<F, std::remove_cvref_t<D>...>;

}  // namespace lv
