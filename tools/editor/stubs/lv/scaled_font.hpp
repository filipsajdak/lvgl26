#pragma once

// Editor stub of lv/scaled_font.hpp: a font whose size changes while the program runs.

#include <lv/style.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <cstddef>

namespace lv {

/// A font that the device draws, with LVGL's TinyTTF: set(LVGL26_TTF ON) in CMake.
///   constinit lv::scaled_font reading{barlow_ttf, tok.text.label};
///   lv::label({.style = {.text_font = lv::bind(reading)}})
class scaled_font {
 public:
  /// At the size of a typography token.
  template <std::size_t N>
  constexpr scaled_font(const unsigned char (&)[N], const token<typography_token>&) {}
  /// At a size that no token names: lv::raw(24 * lv::px).
  template <std::size_t N>
  constexpr scaled_font(const unsigned char (&)[N], raw_value<length>) {}
  template <std::size_t N>
  scaled_font(const unsigned char (&)[N], length) = delete(
      "the size of a font comes from a typography token (tok.text.title). For a size that no token "
      "names, say so: lv::raw(24 * lv::px)");
  scaled_font(const scaled_font&) = delete;
  scaled_font& operator=(const scaled_font&) = delete;
  ~scaled_font();

  /// The size now.
  length size() const;
  /// A new size, from a typography token: the widgets that use the font follow.
  void resize(const token<typography_token>& type);
  /// A new size that no token names: resize(lv::raw(31 * px)).
  void resize(raw_value<length> size);
  /// LVGL's font, made on first use.
  const lv_font_t* raw() const;
  /// Frees the font.
  void reset();
};

/// Binds a font property of a style to a scaled font: .text_font = lv::bind(reading).
constexpr font_binding bind(const scaled_font&) {
  return {};
}

}  // namespace lv
