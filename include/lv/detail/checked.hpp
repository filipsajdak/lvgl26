#pragma once

// Keeps a contract checked where GCC 16.2 drops it.
//
// GCC 16.2 folds a call whose arguments are constants without evaluating its
// contracts, when the call is part of a larger expression:
//
//   struct pause { lv::duration d; };
//   pause p{-1 * ms};   // compiled, and ran, with no check
//
// A constexpr function with a contract on its values repeats the condition as its
// first statement:
//
//   constexpr duration operator*(T n, ms_unit)
//     pre(n >= 0)
//   {
//     detail::keep_checked(n >= 0);
//
// A false condition then reaches a function that is not constexpr, the fold fails,
// and the call is evaluated with its contracts: a compile error in a constant
// expression, a contract violation at run time.

namespace lv::detail {

// Not constexpr on purpose: reaching it ends a constant evaluation.
inline void not_a_constant() {}

constexpr void keep_checked(bool holds) {
  if (!holds) not_a_constant();
}

}  // namespace lv::detail
