#pragma once

// Unit types. LVGL takes a plain integer for pixels, percent, angles, times and
// opacity; these types keep them apart at compile time and cost nothing at run time.

#include <lvgl/lvgl.h>

#include <cstdint>

namespace lv {

// A share of something: 50_pct. Converts to a length or to an opacity.
struct percent {
  std::int32_t value = 0;
};

// A size or position: 120_px, 50_pct or lv::size_content.
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

// An angle in degrees: 90_deg.
struct angle {
  std::int32_t degrees = 0;
  constexpr std::int32_t raw() const { return degrees; }
  friend constexpr bool operator==(angle, angle) = default;
};

// A time span in milliseconds: 300_ms.
struct duration {
  std::uint32_t ms = 0;
  constexpr std::uint32_t raw() const { return ms; }
  friend constexpr bool operator==(duration, duration) = default;
};

// An opacity: 50_pct, lv::opa::cover or lv::opa::transparent.
class opa {
  lv_opa_t raw_ = LV_OPA_COVER;
  constexpr explicit opa(lv_opa_t raw) : raw_(raw) {}

 public:
  constexpr opa() = default;
  constexpr opa(percent p)
      : raw_(static_cast<lv_opa_t>((p.value < 0 ? 0 : p.value > 100 ? 100 : p.value) * 255 / 100)) {}
  static const opa cover;
  static const opa transparent;
  constexpr lv_opa_t raw() const { return raw_; }
  friend constexpr bool operator==(opa, opa) = default;
};
inline constexpr opa opa::cover{};
inline constexpr opa opa::transparent{percent{0}};

inline namespace literals {

// The value must fit. A literal that does not is a compile error.
consteval std::int32_t checked_i32(unsigned long long v) {
  if (v > 0x7fffffffULL) throw "lv: this literal is too large";
  return static_cast<std::int32_t>(v);
}

consteval length operator""_px(unsigned long long v) { return length::px(checked_i32(v)); }
consteval percent operator""_pct(unsigned long long v) { return {checked_i32(v)}; }
consteval angle operator""_deg(unsigned long long v) { return {checked_i32(v)}; }
consteval duration operator""_ms(unsigned long long v) {
  return {static_cast<std::uint32_t>(checked_i32(v))};
}

// A color as 0xRRGGBB: 0x3366ff_rgb.
consteval lv_color_t operator""_rgb(unsigned long long v) {
  if (v > 0xffffffULL) throw "lv: a color literal is 0xRRGGBB";
  lv_color_t c{};
  c.red = static_cast<std::uint8_t>(v >> 16);
  c.green = static_cast<std::uint8_t>(v >> 8);
  c.blue = static_cast<std::uint8_t>(v);
  return c;
}

}  // namespace literals

}  // namespace lv
