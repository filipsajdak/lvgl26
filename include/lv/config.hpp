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

#include <cstddef>

namespace lv {

struct config {
  // Accept plain values such as 8 * lv::px in styles, instead of design tokens only.
  bool raw_style_values = false;

  // Accept lv::state<int> and other plain integers, instead of a named type
  // such as `struct kmh { int value; };`.
  bool raw_integer_state = false;

  // The largest picture, in pixels, that the compiler decodes (see lv/image.hpp).
  // Decoding costs compile time: about a second for 48 x 48, many for a whole screen.
  std::size_t compile_time_image_pixels = 128 * 128;
};

struct override {};

template <class = override>
inline constexpr config cfg{};

}  // namespace lv
