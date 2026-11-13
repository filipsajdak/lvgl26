#pragma once

// Project-wide settings, without macros.
//
// To change a setting, specialize lv::cfg once, in a header that every file of the
// project includes right after <lv/lv.hpp>:
//
//   template <>
//   inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};
//
// The library reads the settings through a template parameter, so the
// specialization is found as long as it comes before the first use. If it comes
// later, the compiler reports a redefinition of lv::cfg<lv::override>.
// (The same technique as boost-ext/ut's `cfg<override>`.)

namespace lv {

struct config {
  // Accept plain values such as 8_px in styles, instead of design tokens only.
  bool raw_style_values = false;
};

struct override {};

template <class = override>
inline constexpr config cfg{};

}  // namespace lv
