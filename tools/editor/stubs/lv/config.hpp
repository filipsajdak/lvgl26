#pragma once

// Editor stub of lv/config.hpp: the project-wide settings.
//
//   template <>
//   inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};

#include <cstddef>

namespace lv {

/// The settings of a project. Change one by specializing lv::cfg<lv::override>.
struct config {
  /// Accept plain values such as 8 * lv::px in styles, instead of design tokens only.
  /// The editor does not read this setting: it marks a plain value in a style either way.
  bool raw_style_values = false;

  /// Accept lv::state<int> and other plain integers, instead of a named type.
  bool raw_integer_state = false;

  /// The largest picture, in pixels, that the compiler decodes (see lv/image.hpp).
  std::size_t compile_time_image_pixels = 128 * 128;
};

/// The tag of the settings: lv::cfg<lv::override>.
struct override {};

/// The settings in use. Specialize it once, right after <lv/lv.hpp>.
template <class = override>
inline constexpr config cfg{};

}  // namespace lv
