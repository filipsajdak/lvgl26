#pragma once

// Signal's typefaces, and the Signal themes that use them.
//
//   #include <lv/signal_type.hpp>     // in one file of the program
//
//   lv::display display{app, frame, lv::signal::dark_with_type};
//
// Signal sets numbers and titles in Barlow Condensed and everything else in Barlow
// (design/signal, "Type"). `dark_with_type` and `day_with_type` are `dark` and `day` of
// lv/signal.hpp with those fonts: all text is `body`, and the text of a button is
// `label`. The compiler draws the two fonts in every file that includes this header:
// about 7 seconds, and 13 KB of flash. That is why it is a header of its own:
// lv/signal.hpp alone draws no font.
//
// The larger styles are yours to make, once each, where you use them:
//
//   constexpr auto title = lv::signal::type(lv::signal::type_tokens.text.title);
//   constexpr auto reading = lv::signal::numerals(lv::signal::type_tokens.text.numeral_l);
//
//   lv::label({.style = {.text_font = reading}, .text = "21.5"})
//
// `type` holds the characters from the space to the tilde. `numerals` holds only what a
// reading shows, so a large number costs little: at 72 px 11 KB instead of 62 KB.
// docs/themes.md has the cost of each style.
//
// The fonts have no glyph for LVGL's symbols: the arrow of a drop-down list is drawn as
// an empty box.

#include <lv/font.hpp>
#include <lv/signal.hpp>

namespace lv::signal {

namespace detail {

constexpr char type_json[] = {
#embed "signal/type.json"
    , 0};

}  // namespace detail

// The files of Barlow that Signal uses, for a typeface of your own with other characters.
// They are under the SIL Open Font License (include/lv/signal/fonts/OFL.txt). A file does
// not reach the program: only the glyphs of the fonts you make do.
// Like the token files they are not `inline`: under a sanitizer GCC 16.2 does not take the
// address of an inline variable for a constant that is not null, and reading text checks that.
constexpr unsigned char barlow_medium_ttf[] = {
#embed "signal/fonts/Barlow-Medium.ttf"
};
constexpr unsigned char barlow_condensed_semibold_ttf[] = {
#embed "signal/fonts/BarlowCondensed-SemiBold.ttf"
};
constexpr unsigned char barlow_condensed_bold_ttf[] = {
#embed "signal/fonts/BarlowCondensed-Bold.ttf"
};

// Signal's typography: `type_tokens.font.display`, `type_tokens.text.title` and so on.
// It is the same in both themes.
inline constexpr auto type_tokens = lv::tokens<detail::type_json>;

// Signal's typeface: the file of every family and weight that the tokens name.
// `type(type_tokens.text.title)` is the font of that style.
inline constexpr lv::typeface type{
    {"Barlow Condensed", 700, barlow_condensed_bold_ttf},      // the numerals and `label`
    {"Barlow Condensed", 600, barlow_condensed_semibold_ttf},  // `title`
    {"Barlow", 500, barlow_medium_ttf},                        // `body` and `caption`
};

// The numeral styles with only the characters of a reading: the digits, the point, the
// colon, the minus, the percent sign, the degree sign and the space. Signal keeps its
// large numbers in such a font (design/signal, "For the renderer"): sixteen glyphs
// instead of ninety-five.
inline constexpr lv::typeface numerals{
    {"Barlow Condensed", 700, barlow_condensed_bold_ttf, "0123456789.:-%\xC2\xB0 "},
};

// The two fonts of the themes, for a style of your own: `.text_font = lv::signal::label_font`.
// Each is drawn once, here, for both themes.
inline constexpr auto body_font = type(type_tokens.text.body);
inline constexpr auto label_font = type(type_tokens.text.label);

inline constexpr lv::theme_styles dark_with_type = detail::themed<dark_tokens>(body_font, label_font);
inline constexpr lv::theme_styles day_with_type = detail::themed<day_tokens>(body_font, label_font);

}  // namespace lv::signal
