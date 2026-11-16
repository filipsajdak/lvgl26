#pragma once

// A font that the device draws from a TrueType file, at a size that can change while
// the program runs. Use it where the size is not known when the program is built; a
// font of a fixed size is better made by the compiler (lv/font.hpp), which costs the
// device no heap and no code.
//
//   constexpr unsigned char barlow_ttf[] = {
//   #embed "Barlow-Medium.ttf"
//   };
//   constinit lv::scaled_font title{barlow_ttf, tok.text.title};   // the size of the token
//
//   lv::label({.style = {.text_font = lv::bind(title)}, .text = "Wake up"})
//
//   title.resize(tok.text.large);       // every widget bound to it follows
//   title.resize(lv::raw(31 * px));     // a size that no token names
//
// It needs LVGL's TinyTTF, which is off by default: `set(LVGL26_TTF ON)` in CMake
// before lvgl26 is added. TinyTTF keeps the glyphs it has drawn on LVGL's heap.
//
// The font is made on first use, after lv::application has started LVGL.

#include <lv/detail/api.hpp>
#include <lv/style.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace lv {

namespace detail {

// LVGL's TinyTTF functions, or null reflections when LVGL is built without it.
// D only delays the search until a font is used.
template <class D = void>
inline constexpr std::meta::info tiny_ttf_create = find("lv_tiny_ttf_create_data");
template <class D = void>
inline constexpr std::meta::info tiny_ttf_set_size = find("lv_tiny_ttf_set_size");
template <class D = void>
inline constexpr std::meta::info tiny_ttf_destroy = find("lv_tiny_ttf_destroy");

// Stops the build of a program that uses a font drawn on the device without TinyTTF.
template <class D>
constexpr void needs_tiny_ttf() {
  static_assert(tiny_ttf_create<D> != std::meta::info{},
                "lv: lv::scaled_font needs LVGL's TinyTTF, which is off. Turn it on in CMake, before lvgl26 "
                "is added: set(LVGL26_TTF ON). A font of a fixed size needs no TinyTTF: see lv/font.hpp.");
}

}  // namespace detail

class scaled_font {
 public:
  // The font of `file` at the size the typography token gives.
  template <std::size_t N>
  constexpr scaled_font(const unsigned char (&file)[N], const token<typography_token>& type)
      : file_(file), px_(type->font_size.raw()) {}

  // The font at a size that no token names.
  template <std::size_t N>
  constexpr scaled_font(const unsigned char (&file)[N], raw_value<length> size) : file_(file), px_(size.value.raw()) {}

  template <std::size_t N>
  scaled_font(const unsigned char (&)[N], length) = delete(
      "the size of a font comes from a typography token (tok.text.title). For a size that no token "
      "names, say so: lv::raw(24 * lv::px)");

  scaled_font(const scaled_font&) = delete;
  scaled_font& operator=(const scaled_font&) = delete;
  ~scaled_font() { reset<>(); }

  length size() const { return length::px(px_); }

  // Changes the size. Every widget whose style is bound to this font is laid out
  // and drawn again.
  template <class D = void>
  void resize(const token<typography_token>& type) {
    resize_to<D>(type->font_size.raw());
  }
  template <class D = void>
  void resize(raw_value<length> size) {
    resize_to<D>(size.value.raw());
  }

  // LVGL's font, made on first use. Null if LVGL has no memory for it.
  template <class D = void>
  const lv_font_t* raw() const {
    detail::needs_tiny_ttf<D>();
    return made<D>();
  }

  // Gives the font back to LVGL. Widgets bound to it must be gone. Called by the destructor.
  template <class D = void>
  void reset() {
    if constexpr (detail::tiny_ttf_destroy<D> != std::meta::info{}) {
      if (font_ != nullptr && lv_is_initialized()) [:detail::tiny_ttf_destroy<D>:](font_);
    }
    font_ = nullptr;
  }

 private:
  friend const lv_font_t* detail::bound_font(const void*);

  // The font, made on first use; null in a build without TinyTTF. A style's code is
  // compiled for every program, so this must compile there too: lv::bind and raw()
  // are where a program that uses a font is told what is missing.
  template <class D = void>
  const lv_font_t* made() const {
    if constexpr (detail::tiny_ttf_create<D> != std::meta::info{}) {
      if (font_ == nullptr) font_ = [:detail::tiny_ttf_create<D>:](file_.data(), file_.size(), px_);
    }
    return font_;
  }

  template <class D>
  void resize_to(std::int32_t px)
    pre(px > 0)
  {
    detail::needs_tiny_ttf<D>();
    px_ = px;
    if constexpr (detail::tiny_ttf_set_size<D> != std::meta::info{}) {
      if (font_ == nullptr) return;  // not used yet: it is made at the new size
      [:detail::tiny_ttf_set_size<D>:](font_, px);
      lv_obj_report_style_change(nullptr);  // widgets hold the font, not its size
    }
  }

  std::span<const unsigned char> file_;
  std::int32_t px_;
  mutable lv_font_t* font_ = nullptr;
};

// Binds a font property of a style to a font: .text_font = lv::bind(title).
// D only delays the look for TinyTTF until a program binds a font.
template <class D = void>
constexpr font_binding bind(const scaled_font& font) {
  detail::needs_tiny_ttf<D>();
  return {&font};
}

namespace detail {

inline const lv_font_t* bound_font(const void* source) { return static_cast<const scaled_font*>(source)->made<>(); }

}  // namespace detail

}  // namespace lv
