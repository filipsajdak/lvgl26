#pragma once

// Editor stub of lv/signal_type.hpp: Signal's typefaces, and its themes that set text in them.

#include <lv/font.hpp>
#include <lv/signal.hpp>
#include <lv/signal_tokens.hpp>
#include <lv/style.hpp>

namespace lv::signal {

/// The files of Barlow that Signal uses, for a typeface of your own with other characters.
inline constexpr unsigned char barlow_medium_ttf[1]{};
inline constexpr unsigned char barlow_condensed_semibold_ttf[1]{};
inline constexpr unsigned char barlow_condensed_bold_ttf[1]{};

/// Signal's typography: type_tokens.text.title, type_tokens.font.display and so on.
inline constexpr lv::stub::signal_type_tokens type_tokens{};

/// Signal's typeface: type(type_tokens.text.title) is the font of that style.
inline constexpr lv::typeface type{lv::face_file{}};

/// Signal's numerals with only the characters of a reading: numerals(type_tokens.text.numeral_l).
inline constexpr lv::typeface numerals{lv::face_file{}};

/// The two fonts of the themes, for a style of your own: .text_font = lv::signal::label_font.
inline constexpr auto body_font = type(type_tokens.text.body);
inline constexpr auto label_font = type(type_tokens.text.label);

/// The letter spacing of `label`, from the token.
inline constexpr auto label_spacing = lv::token<lv::length>{};

/// Signal's dark theme with every text in Barlow: lv::display display{app, frame, lv::signal::dark_with_type}.
inline constexpr lv::theme_styles dark_with_type = lv::stub::theme_access::some();
/// Signal's day theme with every text in Barlow.
inline constexpr lv::theme_styles day_with_type = lv::stub::theme_access::some();

}  // namespace lv::signal
