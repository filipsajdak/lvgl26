#pragma once

// Signal's typefaces, drawn once in a library instead of in every file.
//
//   #include <lv/signal_fonts.hpp>     // and link lvgl26::signal_type
//
//   lv::display display{app, frame, lv::signal::dark_with_type};
//   lv::label({.text = "21.5", .style = {.text_font = lv::font_of<"text.numeral-m", lv::signal::type_design>}})
//
// The names are those of lv/signal_type.hpp, and the fonts are the same: `dark_with_type`,
// `day_with_type`, `body_font`, `label_font`, `type_tokens`. The difference is where the
// compiler draws them. lv/signal_type.hpp draws two fonts in each file that includes it (about
// 168 G instructions of compile time). This header draws none: a font is the address of an
// object that the library `lvgl26::signal_type` defines, one file for each of the eight text
// tokens (lib/signal_type). Include this header or lv/signal_type.hpp in a program, not both.
//
// The library's fonts are the full text from the space to the tilde for `title`, `label`,
// `body` and `caption`, and only what a reading shows (the digits, the point, the colon, the
// minus, the percent sign, the degree sign and the space) for the four `numeral-*` sizes. A
// font nobody names costs no flash: the linker takes the files of the library that the program
// uses. Name any of the eight as above: `lv::font_of<"text.title", lv::signal::type_design>`.

#include <lv/detail/missing.hpp>
#include <lv/look.hpp>
#include <lv/signal.hpp>

static_assert(!lv::detail::included<"lv/signal_type.hpp">,
              "lv: lv/signal_fonts.hpp and lv/signal_type.hpp both give Signal's fonts, one linked and one drawn in each "
              "file. Include one of them.");

namespace lv::detail {
template <>
inline constexpr bool included<"lv/signal_fonts.hpp"> = true;
}  // namespace lv::detail

#include <lv/signal_type_design.hpp>

namespace lv::signal {

// The two fonts of the themes, for a style of your own: `.text_font = lv::signal::label_font`.
inline constexpr auto body_font = lv::font_of<"text.body", type_design>;
inline constexpr auto label_font = lv::font_of<"text.label", type_design>;

// `label` is set with wide spacing: the space between its letters comes from the token too.
inline constexpr auto label_spacing = lv::detail::part(type_tokens.text.label->letter_spacing);

inline constexpr lv::theme_styles dark_with_type = detail::themed<dark_tokens>(body_font, label_font, label_spacing);
inline constexpr lv::theme_styles day_with_type = detail::themed<day_tokens>(body_font, label_font, label_spacing);

}  // namespace lv::signal
