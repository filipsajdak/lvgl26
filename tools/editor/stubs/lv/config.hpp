#pragma once

// Editor stub of lv/config.hpp: the project-wide settings. A project with a settings file,
// lvgl26.json, sets them there; a build without one specializes lv::cfg:
//
//   template <>
//   inline constexpr lv::config lv::cfg<lv::override>{.rules = {.raw_style_values = true}};

#include <cstddef>

namespace lv {

/// The text of a description, as an annotation on a setting.
template <std::size_t N>
struct described {
  char text[N];
  consteval described(const char (&from)[N]) {
    for (std::size_t i = 0; i < N; ++i) text[i] = from[i];
  }
};

/// A unit of a limit in the settings file, and how many of the base unit it is.
struct settings_unit {
  const char* name;
  std::size_t factor;
};

/// A number of pixels: {"value": 16384, "unit": "px"} in the file.
struct pixel_count {
  std::size_t value;
  static constexpr settings_unit units[] = {{"px", 1}};
};

/// A number of bytes: {"value": 4, "unit": "KiB"} in the file.
struct byte_count {
  std::size_t value;
  static constexpr settings_unit units[] = {{"B", 1}, {"KiB", 1024}, {"MiB", 1024 * 1024}};
};

/// What the program may use. Each is allowed by default; false makes a part that needs it a compile error.
struct allow_settings {
  /// Memory from the system heap while the program runs. LVGL's own fixed pool is not the system heap.
  bool allocation = true;
  /// Threads, and the locks between them.
  bool threads = true;
  /// Files and the standard streams.
  bool files = true;
  /// Floating-point arithmetic while the program runs.
  bool floating_point = true;
};

/// The library's own strictness. Each is strict by default; true relaxes it.
struct rule_settings {
  /// Plain values such as 8 * lv::px in styles. The editor marks a plain value in a style either way.
  bool raw_style_values = false;
  /// A state, a computed value or a row field of a plain integer.
  bool raw_integer_state = false;
  /// The forms of a token value from before the Design Tokens Format 2025.10, while a project's files convert.
  bool tokens_legacy_values = false;
};

/// What the compiler may do.
struct limit_settings {
  /// The largest picture that the compiler decodes.
  pixel_count compile_time_image{128 * 128};
};

/// The settings of a project.
struct config {
  allow_settings allow{};
  rule_settings rules{};
  limit_settings limits{};
};

/// The tag of the settings: lv::cfg<lv::override>.
struct override {};

/// The text of a project's settings file, and the name the project gave it.
struct settings_text {
  const char* text;
  std::size_t size;
  const char* name;
};

/// The settings file of the project: written by lvgl26_settings() in CMake.
template <class = override>
inline constexpr settings_text settings_file{nullptr, 0, nullptr};

/// The settings in a build without a file. Specialize it once, right after <lv/lv.hpp>.
template <class = override>
inline constexpr config cfg{};

}  // namespace lv
