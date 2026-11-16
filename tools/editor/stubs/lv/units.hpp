#pragma once

// Editor stub of lv/units.hpp: the unit types, as plain C++ that clangd can read.
// The compiler that builds a program reads include/lv/units.hpp, never this file.

#include <lvgl/lvgl.h>

#include <compare>
#include <concepts>
#include <cstdint>
#include <utility>

namespace lv {

/// A share of something: 50 * pct. Converts to a length or to an opacity.
struct percent {
  std::int32_t value = 0;
};

/// A size or position: 120 * px, 50 * pct or lv::size_content.
class length {
  std::int32_t raw_ = 0;
  constexpr explicit length(std::int32_t raw) : raw_(raw) {}

 public:
  constexpr length() = default;
  constexpr length(percent p) : raw_(LV_PCT(p.value)) {}
  /// A length in pixels: what `120 * px` makes.
  static constexpr length px(std::int32_t v) { return length{v}; }
  /// As large as the content needs: lv::size_content.
  static constexpr length content() { return length{LV_SIZE_CONTENT}; }
  /// The number LVGL takes.
  constexpr std::int32_t raw() const { return raw_; }
  friend constexpr bool operator==(length, length) = default;
};

/// As large as the content needs.
inline constexpr length size_content = length::content();

struct decidegrees;

/// An angle in whole degrees: 90 * deg. Most of LVGL takes it (an arc, a scale).
struct angle {
  std::int32_t degrees = 0;
  constexpr std::int32_t raw() const { return degrees; }
  constexpr operator decidegrees() const;
  friend constexpr bool operator==(angle, angle) = default;
};

/// An angle in tenths of a degree: 22.5 * deg, or any whole angle such as 90 * deg.
/// The rotation of an image and the transform rotation and skew of a style take it.
struct decidegrees {
  std::int32_t tenths = 0;
  constexpr std::int32_t raw() const { return tenths; }

  /// Degrees as a number, as a design token or a document gives them: 22.5.
  static constexpr decidegrees from_number(double degrees) {
    return {static_cast<std::int32_t>(degrees * 10 + (degrees < 0 ? -0.5 : 0.5))};
  }

  operator angle() const = delete(
      "lv: this takes an angle in whole degrees, and the value may have a fraction of a degree "
      "(it is lv::decidegrees, as 22.5 * lv::deg is). Write whole degrees: 90 * lv::deg.");

  friend constexpr bool operator==(decidegrees, decidegrees) = default;
  friend constexpr bool operator==(decidegrees a, angle b) { return a.tenths == std::int64_t{b.degrees} * 10; }
};

constexpr angle::operator decidegrees() const {
  return {degrees * 10};
}

/// A time span in milliseconds: 300 * ms, 2 * sec.
struct duration {
  std::uint32_t ms = 0;
  constexpr std::uint32_t raw() const { return ms; }
  friend constexpr bool operator==(duration, duration) = default;
};

/// An opacity: 50 * pct, lv::opa::cover or lv::opa::transparent.
class opa {
  lv_opa_t raw_ = LV_OPA_COVER;
  constexpr explicit opa(lv_opa_t raw) : raw_(raw) {}

 public:
  constexpr opa() = default;
  constexpr opa(percent p)
      : raw_(static_cast<lv_opa_t>((p.value < 0 ? 0 : p.value > 100 ? 100 : p.value) * 255 / 100)) {}
  /// An opacity from LVGL's number, 0 to 255.
  static constexpr opa from_raw(lv_opa_t raw) { return opa{raw}; }
  /// Fully opaque.
  static const opa cover;
  /// Fully see-through.
  static const opa transparent;
  constexpr lv_opa_t raw() const { return raw_; }
  friend constexpr bool operator==(opa, opa) = default;
};
inline constexpr opa opa::cover{};
inline constexpr opa opa::transparent{percent{0}};

struct x_axis;
struct y_axis;

namespace detail {
template <class T>
concept axis = std::same_as<T, x_axis> || std::same_as<T, y_axis>;
}  // namespace detail

/// A horizontal or a vertical length. A plain length converts to either; a value that
/// was named as one kind cannot be passed where the other is expected.
template <detail::axis Axis>
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

/// A width or an x position.
using horizontal = along<x_axis>;
/// A height or a y position.
using vertical = along<y_axis>;

namespace detail {

template <class T>
concept unit_count = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char>;

}  // namespace detail

/// Units, written as in mp-units: a number times a unit.
///   320 * px    50 * pct    90 * deg    22.5 * deg    300 * ms    2 * sec
inline namespace units {

/// Pixels: 320 * px is an lv::length.
inline constexpr struct px_unit {
} px;
/// Percent: 50 * pct is a share of the parent's size, or an opacity.
inline constexpr struct pct_unit {
} pct;
/// Degrees: 90 * deg is an lv::angle; 22.5 * deg is an lv::decidegrees.
inline constexpr struct deg_unit {
} deg;
/// Milliseconds: 300 * ms is an lv::duration.
inline constexpr struct ms_unit {
} ms;
/// Seconds: 2 * sec is an lv::duration.
inline constexpr struct sec_unit {
} sec;

template <detail::unit_count T>
constexpr length operator*(T n, px_unit) {
  return length::px(static_cast<std::int32_t>(n));
}
template <detail::unit_count T>
constexpr percent operator*(T n, pct_unit) {
  return {static_cast<std::int32_t>(n)};
}
template <detail::unit_count T>
constexpr angle operator*(T n, deg_unit) {
  return {static_cast<std::int32_t>(n)};
}
template <std::same_as<double> T>
constexpr decidegrees operator*(T n, deg_unit) {
  return decidegrees::from_number(n);
}
template <detail::unit_count T>
constexpr duration operator*(T n, ms_unit) {
  return {static_cast<std::uint32_t>(n)};
}
template <detail::unit_count T>
constexpr duration operator*(T n, sec_unit) {
  return {static_cast<std::uint32_t>(n) * 1000};
}

}  // namespace units

/// A count of things: clicks, items, steps.
///   constinit lv::state clicks{lv::counter{}};
struct counter {
  std::int32_t value = 0;

  constexpr counter& operator++() {
    ++value;
    return *this;
  }
  constexpr counter& operator--() {
    --value;
    return *this;
  }
  constexpr counter& operator+=(std::int32_t n) {
    value += n;
    return *this;
  }
  constexpr counter& operator-=(std::int32_t n) {
    value -= n;
    return *this;
  }
  friend constexpr auto operator<=>(const counter&, const counter&) = default;
};

/// A color from 0xRRGGBB: lv::rgb(0x3366ff).
constexpr lv_color_t rgb(std::uint32_t hex) {
  lv_color_t c{};
  c.red = static_cast<std::uint8_t>(hex >> 16);
  c.green = static_cast<std::uint8_t>(hex >> 8);
  c.blue = static_cast<std::uint8_t>(hex);
  return c;
}

}  // namespace lv
