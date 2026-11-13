#pragma once

// lv::match: choose by value, with every case checked at compile time.
//
//   enum class mode { off, eco, sport };
//
//   const char* name = lv::match(m)(
//       lv::is<mode::off>.then([] { return "Off"; }),
//       lv::is<mode::eco>.then([] { return "Eco"; }),
//       lv::is<mode::sport>.then([] { return "Sport"; }));
//
//   int level = lv::match(n)(
//       lv::is<1, 2>.then([] { return 0; }),    // 1 or 2
//       lv::otherwise.then([] { return 1; }));
//
// It follows the pattern matching proposed for C++29 (P2688, `match` and `case`) as
// far as a library can: one arm per case, alternatives in one arm, a wildcard, one
// result type, and hard errors for a case that is missing or given twice.
// For an enum, reflection checks that every enumerator is covered; the error names
// the ones that are not. For any other type the match needs an lv::otherwise arm,
// except bool, which is covered by true and false.
// A handler takes the matched value, or nothing.

#include <lv/detail/api.hpp>

#include <concepts>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv {

namespace detail {

// A pattern with what to do when it matches.
template <class Pattern, class F>
struct arm {
  F handler;
};

// The pattern "equal to one of these values".
template <auto... Values>
struct one_of {
  // What to do when the value is one of these: lv::is<1, 2>.then([] { ... }).
  template <std::move_constructible F>
  constexpr arm<one_of, F> then(F handler) const {
    return {std::move(handler)};
  }
};

// The pattern "anything else".
struct anything {
  // What to do for every other value: lv::otherwise.then([] { ... }).
  template <std::move_constructible F>
  constexpr arm<anything, F> then(F handler) const {
    return {std::move(handler)};
  }
};

template <class T>
inline constexpr bool is_arm = false;
template <class Pattern, class F>
inline constexpr bool is_arm<arm<Pattern, F>> = true;

// A handler of a match on Subject: it takes the matched value, or nothing.
template <class F, class Subject>
concept handler_for = std::invocable<F&, const Subject&> || std::invocable<F&>;

// An arm of a match on Subject: a pattern with a handler for that subject.
template <class A, class Subject>
concept arm_for =
    is_arm<A> && requires(A& a) { requires handler_for<decltype(a.handler), Subject>; };

template <class T>
inline constexpr bool is_wildcard = false;
template <class F>
inline constexpr bool is_wildcard<arm<anything, F>> = true;

template <class T, class Subject>
constexpr decltype(auto) run_arm(T& a, const Subject& subject) {
  if constexpr (requires { a.handler(subject); })
    return a.handler(subject);
  else
    return a.handler();
}

template <class... R>
struct match_result : std::common_type<R...> {};
template <class... R>
  requires(std::is_void_v<R> && ...)
struct match_result<R...> {
  using type = void;
};

// How a value is written in an error message: the enumerator's name, or the number.
template <class Subject>
consteval std::string value_text(Subject v) {
  if constexpr (std::is_enum_v<Subject>) {
    for (auto e : std::meta::enumerators_of(^^Subject))
      if (std::meta::extract<Subject>(e) == v) return str(std::meta::identifier_of(e));
    return "(not an enumerator)";
  } else if constexpr (std::same_as<Subject, bool>) {
    return v ? "true" : "false";
  } else if constexpr (std::integral<Subject>) {
    std::string out;
    auto n = static_cast<long long>(v);
    const bool negative = n < 0;
    if (n == 0) out = "0";
    for (auto m = negative ? -n : n; m > 0; m /= 10) out.insert(out.begin(), static_cast<char>('0' + m % 10));
    return negative ? "-" + out : out;
  } else {
    return "(a value)";
  }
}

// Collects the values of an arm's pattern, converted to the subject's type.
template <class Subject, auto... Values, class F>
consteval void add_values(std::vector<Subject>& out, std::type_identity<arm<one_of<Values...>, F>>) {
  (out.push_back(static_cast<Subject>(Values)), ...);
}
template <class Subject, class F>
consteval void add_values(std::vector<Subject>&, std::type_identity<arm<anything, F>>) {}

// What is wrong with the arms of a match on Subject, or an empty string.
template <class Subject, class... Arms>
consteval std::string_view match_problem() {
  constexpr bool wildcard = (is_wildcard<Arms> || ...);
  constexpr bool wildcard_is_last = is_wildcard<Arms...[sizeof...(Arms) - 1]>;
  constexpr int wildcards = (int{is_wildcard<Arms>} + ...);
  if (wildcards > 1) return "lv::match has more than one lv::otherwise arm.";
  if (wildcard && !wildcard_is_last)
    return "lv::match: lv::otherwise must be the last arm; the arms after it can never match.";

  std::vector<Subject> covered;
  (add_values(covered, std::type_identity<Arms>{}), ...);
  for (std::size_t i = 0; i < covered.size(); ++i)
    for (std::size_t j = i + 1; j < covered.size(); ++j)
      if (covered[i] == covered[j])
        return std::define_static_string("lv::match has two arms for " + value_text(covered[i]) +
                                         ". Each value needs exactly one.");

  // The values a subject of this type can have, when they can be listed.
  std::vector<Subject> all;
  bool listable = false;
  if constexpr (std::is_enum_v<Subject>) {
    listable = true;
    for (auto e : std::meta::enumerators_of(^^Subject)) {
      const auto v = std::meta::extract<Subject>(e);
      bool known = false;  // two enumerators may share a value
      for (auto a : all) known = known || a == v;
      if (!known) all.push_back(v);
    }
  } else if constexpr (std::same_as<Subject, bool>) {
    listable = true;
    all = {false, true};
  }

  if (!listable)
    return wildcard ? std::string_view{}
                    : "lv::match on this type cannot list every value, so it needs a last arm "
                      "`lv::otherwise.then(...)`.";

  std::string missing;
  for (auto v : all) {
    bool found = false;
    for (auto c : covered) found = found || c == v;
    if (!found) missing += (missing.empty() ? "" : ", ") + value_text(v);
  }
  if (!wildcard && !missing.empty())
    return std::define_static_string("lv::match does not cover: " + missing +
                                     ". Add an arm for each, or a last arm `lv::otherwise.then(...)`.");
  if (wildcard && missing.empty())
    return "lv::match already covers every value, so its lv::otherwise arm can never match. "
           "Remove it.";
  return {};
}

template <std::equality_comparable Subject>
struct matcher {
  const Subject& subject;

  // Only on the temporary that lv::match returns: the matcher holds a reference to
  // the value, so it must not outlive the expression it was made in.
  template <class... Arms>
    requires(sizeof...(Arms) > 0) && (arm_for<Arms, Subject> && ...)
  constexpr auto operator()(Arms... arms) && {
    static_assert(match_problem<Subject, Arms...>().empty(), match_problem<Subject, Arms...>());
    // Precondition: the value has an arm. With every value covered, only an enum
    // holding a value that is none of its enumerators fails it.
    // A check in the body, not pre(): on GCC 16.2, pre() crashes with a deduced
    // return type, and a fold over the parameters in pre() is not a constant.
    contract_assert((matches(arms) || ...));
    using result = typename match_result<decltype(run_arm(arms, subject))...>::type;
    return choose<result>(arms...);
  }

  // The same signature without the constraint: chosen only when the one above does
  // not apply, to say why.
  template <class... Wrong>
  void operator()(Wrong...) && = delete(
      "lv::match takes arms: lv::is<...>.then(handler) or lv::otherwise.then(handler). "
      "A handler takes the matched value or nothing.");

  template <class... Any>
  void operator()(Any...) const& = delete(
      "lv::match(value)(arms...) must be one expression: the matcher refers to the value "
      "and must not be stored.");

 private:
  template <auto... Values, class F>
  constexpr bool matches(const arm<one_of<Values...>, F>&) const {
    return ((subject == static_cast<Subject>(Values)) || ...);
  }
  template <class F>
  constexpr bool matches(const arm<anything, F>&) const {
    return true;
  }

  template <class Result, class First, class... Rest>
  constexpr Result choose(First& first, Rest&... rest) const {
    if (matches(first)) return static_cast<Result>(run_arm(first, subject));
    if constexpr (sizeof...(Rest) > 0)
      return choose<Result>(rest...);
    else
      std::unreachable();  // excluded by the precondition in operator()
  }
};

}  // namespace detail

// The pattern "equal to this value", or to one of several: lv::is<1, 2>.
template <auto... Values>
  requires(sizeof...(Values) > 0)
inline constexpr detail::one_of<Values...> is{};

// The pattern "anything else". It must be the last arm.
inline constexpr detail::anything otherwise{};

// lv::match(value)(arms...). See the top of this file.
template <std::equality_comparable Subject>
constexpr detail::matcher<Subject> match(const Subject& subject) {
  return {subject};
}

}  // namespace lv
