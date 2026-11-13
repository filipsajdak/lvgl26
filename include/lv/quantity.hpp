#pragma once

// Your own units, in one line each:
//
//   inline constexpr lv::unit<"km/h"> kmh;
//   inline constexpr lv::unit<"rpm"> rpm;
//
//   constinit lv::state speed{0 * kmh};      // lv::state<lv::quantity<"km/h">>
//   speed = 120 * kmh;
//   speed = 3000 * rpm;                      // error: not the same unit
//
//   lv::label({.text = lv::bind(speed)})     // shows "120 km/h"
//
// A quantity is a number that remembers its unit in its type. It is meant for the
// values an application shows: speed, temperature, pressure. It does not convert
// between units; for that, use mp-units (see lv/mp_units.hpp).

#include <lv/detail/fixed_string.hpp>
#include <lv/units.hpp>

#include <compare>
#include <concepts>

namespace lv {

// A number of the unit `Symbol`. Two quantities mix only if their units are the same.
template <detail::fixed Symbol, class Rep = int>
struct quantity {
  Rep value{};

  static constexpr std::string_view symbol = Symbol.sv();

  friend constexpr auto operator<=>(const quantity&, const quantity&) = default;

  constexpr quantity operator-() const { return {static_cast<Rep>(-value)}; }
  friend constexpr quantity operator+(quantity a, quantity b) {
    return {static_cast<Rep>(a.value + b.value)};
  }
  friend constexpr quantity operator-(quantity a, quantity b) {
    return {static_cast<Rep>(a.value - b.value)};
  }
  constexpr quantity& operator+=(quantity b) { return *this = *this + b; }
  constexpr quantity& operator-=(quantity b) { return *this = *this - b; }
};

// A unit: `inline constexpr lv::unit<"km/h"> kmh;`. Multiply a number by it.
// Rep is the integer type that holds the number; int unless you say otherwise.
template <detail::fixed Symbol, class Rep = int>
struct unit {};

template <detail::unit_count T, detail::fixed Symbol, class Rep>
constexpr quantity<Symbol, Rep> operator*(T n, unit<Symbol, Rep>) {
  contract_assert(std::in_range<Rep>(n));
  return {static_cast<Rep>(n)};
}

namespace detail {

template <class T>
inline constexpr bool is_quantity = false;
template <fixed Symbol, class Rep>
inline constexpr bool is_quantity<quantity<Symbol, Rep>> = true;

// "%d km/h": how a quantity is shown by a text binding without a format.
template <class Q>
inline constexpr const char* quantity_format = nullptr;
template <fixed Symbol, class Rep>
inline constexpr const char* quantity_format<quantity<Symbol, Rep>> = [] {
  std::string f = "%d ";
  for (char c : Symbol.sv()) {
    f.push_back(c);
    if (c == '%') f.push_back('%');
  }
  return std::define_static_string(f);
}();

}  // namespace detail

}  // namespace lv
