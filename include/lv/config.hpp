#pragma once

// Project-wide settings, without macros.
//
// A project built with CMake names one JSON file, and every file of the program reads it:
//
//   lvgl26_settings(app lvgl26.json)
//
//   {
//     "allow": {"threads": false, "files": false},
//     "rules": {"raw_style_values": true},
//     "limits": {"compile_time_image": {"value": 16384, "unit": "px"}}
//   }
//
// `allow` says what the program may use; everything is allowed by default. `rules` relaxes
// the library's own strictness; everything is strict by default. `limits` bounds what the
// compiler does. A key the file does not know, a wrong type and an unknown unit are compile
// errors that list what is valid. docs/reference/settings.json describes every key.
//
// A build without the file specializes lv::cfg once, in a header that every file of the
// project includes right after <lv/lv.hpp>:
//
//   template <>
//   inline constexpr lv::config lv::cfg<lv::override>{.rules = {.raw_style_values = true}};
//
// The library reads the settings through a template parameter, so the specialization is
// found as long as it comes before the first use. If it comes later, the compiler reports a
// specialization of lv::cfg<lv::override> after instantiation. A project with a settings file
// has no such specialization: the file's header defines one, so a second is a redefinition.
// (The same technique as boost-ext/ut's `cfg<override>`.)
//
// This header is included into every file of a program that names a settings file, so it
// holds the types and nothing that costs the compiler work.

#include <cstddef>

namespace lv {

// The text of a description, as an annotation: [[=lv::described{"Threads, and locks."}]].
// An annotation holds a structural type, so the text is a character array.
template <std::size_t N>
struct described {
  char text[N];

  consteval described(const char (&from)[N]) {
    for (std::size_t i = 0; i < N; ++i) text[i] = from[i];
  }
};

// A unit of a limit in the settings file, and how many of the base unit it is.
struct settings_unit {
  const char* name;
  std::size_t factor;
};

// A number of pixels: {"value": 16384, "unit": "px"} in the file.
struct pixel_count {
  std::size_t value;

  static constexpr settings_unit units[] = {{"px", 1}};
};

// A number of bytes: {"value": 4, "unit": "KiB"} in the file. A fraction is accepted when it
// gives whole bytes: 0.5 KiB is 512 B.
struct byte_count {
  std::size_t value;

  static constexpr settings_unit units[] = {{"B", 1}, {"KiB", 1024}, {"MiB", 1024 * 1024}};
};

// What the program may use. Each is allowed by default; false forbids it, and a part of
// lvgl26 that needs it is then a compile error that names the key.
struct allow_settings {
  [[=described{"Memory from the system heap while the program runs: new, malloc, a standard container that "
               "grows, the frame of a task. LVGL's own fixed pool is not the system heap."}]]
  bool allocation = true;
  [[=described{"Threads, and the locks between them."}]]
  bool threads = true;
  [[=described{"Files and the standard streams: fopen, stdin, stdout."}]]
  bool files = true;
  [[=described{"Floating-point arithmetic while the program runs. The compiler still uses it to make fonts, "
               "pictures and tokens."}]]
  bool floating_point = true;
};

// The library's own strictness. Each is strict by default; true relaxes it.
struct rule_settings {
  [[=described{"Plain values such as 8 * lv::px in styles, beside design tokens."}]]
  bool raw_style_values = false;
  [[=described{"A state, a computed value or a row field of a plain integer, beside a quantity, an "
               "lv::counter or a named type."}]]
  bool raw_integer_state = false;
  [[=described{"The forms of a token value from before the Design Tokens Format 2025.10: \"#3366ff\" for a color, "
               "\"8px\" or a bare number for a dimension, \"200ms\" for a duration. Only for the time a project's files "
               "convert: tools/dtcg_fix.py rewrites a token file."}]]
  bool tokens_legacy_values = false;
};

// What the compiler may do.
struct limit_settings {
  [[=described{"The largest picture that the compiler decodes. Decoding costs compile time: about a second "
               "for 48 x 48, many for a whole screen."}]]
  pixel_count compile_time_image{128 * 128};
};

struct config {
  [[=described{"What the program may use. Everything is allowed by default."}]]
  allow_settings allow{};
  [[=described{"The library's own strictness. Everything is strict by default."}]]
  rule_settings rules{};
  [[=described{"What the compiler may do."}]]
  limit_settings limits{};
};

struct override {};

// The text of a project's settings file, and the name the project gave it. Empty: no file.
struct settings_text {
  const char* text;
  std::size_t size;
  const char* name;
};

template <class = override>
inline constexpr settings_text settings_file{nullptr, 0, nullptr};

template <class = override>
inline constexpr config cfg{};

}  // namespace lv
