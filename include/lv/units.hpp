#pragma once

// Unit types. LVGL takes a plain integer for pixels, percent, angles, times and
// opacity; these types keep them apart at compile time and cost nothing at run time.

#include <lvgl/lvgl.h>

#include <concepts>
#include <cstdint>
#include <utility>

namespace lv {

// A share of something: 50 * pct. Converts to a length or to an opacity.
struct percent {
  std::int32_t value = 0;
};

// A size or position: 120 * px, 50 * pct or lv::size_content.
class length {
  std::int32_t raw_ = 0;
  constexpr explicit length(std::int32_t raw) : raw_(raw) {}

 public:
  constexpr length() = default;
  constexpr length(percent p) : raw_(LV_PCT(p.value)) {}
  static constexpr length px(std::int32_t v) { return length{v}; }
  static constexpr length content() { return length{LV_SIZE_CONTENT}; }
  constexpr std::int32_t raw() const { return raw_; }
  friend constexpr bool operator==(length, length) = default;
};

// As large as the content needs.
inline constexpr length size_content = length::content();

// An angle in degrees: 90 * deg.
struct angle {
  std::int32_t degrees = 0;
  constexpr std::int32_t raw() const { return degrees; }
  friend constexpr bool operator==(angle, angle) = default;
};

// A time span in milliseconds: 300 * ms.
struct duration {
  std::uint32_t ms = 0;
  constexpr std::uint32_t raw() const { return ms; }
  friend constexpr bool operator==(duration, duration) = default;
};

// An opacity: 50 * pct, lv::opa::cover or lv::opa::transparent.
class opa {
  lv_opa_t raw_ = LV_OPA_COVER;
  constexpr explicit opa(lv_opa_t raw) : raw_(raw) {}

 public:
  constexpr opa() = default;
  constexpr opa(percent p)
      : raw_(static_cast<lv_opa_t>((p.value < 0 ? 0 : p.value > 100 ? 100 : p.value) * 255 / 100)) {}
  static constexpr opa from_raw(lv_opa_t raw) { return opa{raw}; }
  static const opa cover;
  static const opa transparent;
  constexpr lv_opa_t raw() const { return raw_; }
  friend constexpr bool operator==(opa, opa) = default;
};
inline constexpr opa opa::cover{};
inline constexpr opa opa::transparent{percent{0}};

// A horizontal or vertical length: a kind of length, as in mp-units. A plain length
// converts to either, so `.size = {320 * px, 240 * px}` just works; but a value that
// was named as one kind cannot be passed where the other is expected.
template <class Axis>
class along {
  length v_{};

 public:
  constexpr along() = default;

  template <class U>
    requires std::convertible_to<U, length>
  constexpr along(U&& u) : v_(static_cast<length>(std::forward<U>(u))) {}

  constexpr length get() const { return v_; }
  constexpr std::int32_t raw() const { return v_.raw(); }
  friend constexpr bool operator==(along, along) = default;
};

struct x_axis;
struct y_axis;
using horizontal = along<x_axis>;  // a width or an x position
using vertical = along<y_axis>;    // a height or a y position

namespace detail {

template <class T>
concept unit_count = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char>;

// The count as an int32_t. A count that does not fit is a contract violation, and
// so a compile error in a constant expression.
template <unit_count T>
constexpr std::int32_t count(T n) {
  contract_assert(std::in_range<std::int32_t>(n));
  return static_cast<std::int32_t>(n);
}

}  // namespace detail

// Units, written as in mp-units: a number times a unit.
//   320 * px    50 * pct    90 * deg    300 * ms    2 * sec
inline namespace units {

inline constexpr struct px_unit {
} px;
inline constexpr struct pct_unit {
} pct;
inline constexpr struct deg_unit {
} deg;
inline constexpr struct ms_unit {
} ms;
inline constexpr struct sec_unit {
} sec;

template <detail::unit_count T>
constexpr length operator*(T n, px_unit) {
  return length::px(detail::count(n));
}
template <detail::unit_count T>
constexpr percent operator*(T n, pct_unit) {
  return {detail::count(n)};
}
template <detail::unit_count T>
constexpr angle operator*(T n, deg_unit) {
  return {detail::count(n)};
}
template <detail::unit_count T>
constexpr duration operator*(T n, ms_unit) {
  contract_assert(n >= 0);
  return {static_cast<std::uint32_t>(detail::count(n))};
}
template <detail::unit_count T>
constexpr duration operator*(T n, sec_unit) {
  contract_assert(n >= 0 && n <= 2'000'000);
  return {static_cast<std::uint32_t>(detail::count(n)) * 1000};
}

}  // namespace units

// A color from 0xRRGGBB: lv::rgb(0x3366ff).
constexpr lv_color_t rgb(std::uint32_t hex) {
  contract_assert(hex <= 0xffffff);
  lv_color_t c{};
  c.red = static_cast<std::uint8_t>(hex >> 16);
  c.green = static_cast<std::uint8_t>(hex >> 8);
  c.blue = static_cast<std::uint8_t>(hex);
  return c;
}

}  // namespace lv
