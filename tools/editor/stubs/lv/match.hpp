#pragma once

// Editor stub of lv/match.hpp: choosing by value, checked at compile time.
//
//   const char* name = lv::match(m)(
//       lv::is<mode::off>.then([] { return "Off"; }),
//       lv::otherwise.then([] { return "On"; }));
//
// The editor does not check that every value has an arm; GCC does.

#include <concepts>
#include <type_traits>
#include <utility>

namespace lv {

namespace detail {

template <class Pattern, class F>
struct arm {
  F handler;
};

template <auto... Values>
struct one_of {
  /// What to do when the value is one of these: lv::is<1, 2>.then([] { ... }).
  template <std::move_constructible F>
  constexpr arm<one_of, F> then(F handler) const {
    return {std::move(handler)};
  }
};

struct anything {
  /// What to do for every other value: lv::otherwise.then([] { ... }).
  template <std::move_constructible F>
  constexpr arm<anything, F> then(F handler) const {
    return {std::move(handler)};
  }
};

template <class F, class Subject>
struct arm_result : std::invoke_result<F&> {};
template <class F, class Subject>
  requires std::invocable<F&, const Subject&>
struct arm_result<F, Subject> : std::invoke_result<F&, const Subject&> {};

template <class... R>
struct match_result : std::common_type<R...> {};
template <class... R>
  requires(std::is_void_v<R> && ...)
struct match_result<R...> {
  using type = void;
};

template <class Subject>
struct matcher {
  const Subject& subject;

  /// The arms, in order: the handler of the arm that matches runs, and its result is the result.
  template <class... Pattern, class... F>
    requires(sizeof...(F) > 0)
  constexpr typename match_result<typename arm_result<F, Subject>::type...>::type operator()(
      arm<Pattern, F>... arms) && { __builtin_unreachable(); }
};

}  // namespace detail

/// The pattern "equal to one of these values": lv::is<mode::eco>, lv::is<1, 2>.
template <auto... Values>
inline constexpr detail::one_of<Values...> is{};

/// The pattern "anything else". It is the last arm.
inline constexpr detail::anything otherwise{};

/// Chooses by the value of `subject`: lv::match(m)(arm, arm, ...). For an enum, every
/// enumerator needs exactly one arm, or a last lv::otherwise.
template <std::equality_comparable Subject>
constexpr detail::matcher<Subject> match(const Subject& subject) {
  return {subject};
}

}  // namespace lv
