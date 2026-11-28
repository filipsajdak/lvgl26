#pragma once

// Editor stub of lv/icons.hpp.

#include <lv/detail/fixed_string.hpp>
#include <lv/detail/missing.hpp>
#include <lv/tree.hpp>

#include <string_view>

namespace lv {

/// The text of one of Phosphor's icons, for `.text` of a label: lv::glyph<"gear">.
template <detail::fixed Name>
inline constexpr const char* glyph = nullptr;

/// The characters of several icons, for a typeface: lv::glyphs<"arrow-left", "gear">.
template <detail::fixed... Names>
inline constexpr std::string_view glyphs{};

/// A label that shows an icon: lv::icon<"gear">({.style = {.text_font = icon_font}}).
template <detail::fixed Name, class D = void>
constexpr node<"label"> icon(const props<"label">& p = {}) {
  return {p.id};
}

}  // namespace lv
