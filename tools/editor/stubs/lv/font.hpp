#pragma once

// Editor stub of lv/font.hpp: fonts that the compiler makes from TrueType files.

#include <lv/detail/embedded.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace lv {

namespace detail {

template <const auto& File>
struct font_file {
  /// The font at `size`, with `characters` (UTF-8), or with the space to the tilde.
  consteval const lv_font_t* operator()(length, std::string_view = {}) const { return nullptr; }
};

}  // namespace detail

/// The embedded TrueType file `File`: lv::font<barlow_ttf>(18 * px) is a font of it.
/// A style takes such a font only as lv::raw(...).
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr detail::font_file<File> font{};

/// The characters from the space to the tilde and `Extra` on top of them, for the characters of a font:
/// lv::ascii_plus<"€·"> for a price and a separator.
template <detail::fixed Extra>
inline constexpr std::string_view ascii_plus{};

/// The default characters and the accents of Latin-1 (U+00A0 to U+00FF), 191 characters, and `Extra` on top:
/// lv::latin1_plus<"€">. It is the dear one: about 86 G instructions of compile time and 12 KB of flash for each
/// font at 16 px.
template <detail::fixed Extra>
inline constexpr std::string_view latin1_plus{};

/// The default characters and the accents of Latin-1: the cost of lv::latin1_plus<"">.
struct latin1_set {
  constexpr operator std::string_view() const { return {}; }
};
inline constexpr latin1_set latin1{};

/// One file of a typeface: {"Barlow", 500, barlow_ttf}, with the characters to keep
/// as an optional fourth value.
struct face_file {
  std::string_view family;
  std::int32_t weight;
  std::string_view characters;  // UTF-8; empty: the space to the tilde

  template <std::size_t N>
  constexpr face_file(std::string_view family_name, std::int32_t font_weight, const unsigned char (&)[N],
                      std::string_view only = {})
      : family(family_name), weight(font_weight), characters(only) {}
  constexpr face_file() : weight(0) {}
};

/// Says which file each family and weight is: lv::typeface({{"Barlow", 500, barlow_ttf}}).
/// type(tok.text.title) is then the font that the typography token asks for.
class typeface {
 public:
  /// A design without fonts: its looks hold no typography token.
  consteval typeface() {}
  consteval typeface(std::initializer_list<face_file>) {}

  /// The font of a typography token: its family, weight and size.
  consteval token<const lv_font_t*> operator()(const token<typography_token>&) const { return {}; }
};

}  // namespace lv
