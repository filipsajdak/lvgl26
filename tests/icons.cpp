// lv/icons.hpp: the names of Phosphor's icons as text, checked where they are written.
#include <lv/icons.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <string_view>

namespace {

using namespace std::string_view_literals;

// An icon is one character of the private use area, in UTF-8: three bytes.
static_assert(std::string_view{lv::glyph<"arrow-left">} == "\xee\x81\x98"sv);  // U+E058
static_assert(std::string_view{lv::glyph<"check">} == "\xee\x86\x82"sv);       // U+E182
static_assert(std::string_view{lv::glyph<"gear">} == "\xee\x89\xb0"sv);        // U+E270
static_assert(std::string_view{lv::glyph<"plus">} == "\xee\x8f\x94"sv);        // U+E3D4
static_assert(std::string_view{lv::glyph<"x">} == "\xee\x93\xb6"sv);           // U+E4F6
static_assert(std::string_view{lv::glyph<"caret-down">} == "\xee\x84\xb6"sv);  // U+E136

// The first and the last of the table, and a name that is the beginning of another.
static_assert(std::string_view{lv::glyph<"acorn">} == "\xee\xae\x9a"sv);       // U+EB9A
static_assert(lv::detail::code_of_icon("activity") == 0xe000);
static_assert(lv::detail::code_of_icon("arrow-line-down") == 0xe05c);
static_assert(lv::detail::code_of_icon("arrow-line-down-left") == 0xe05e);
static_assert(lv::detail::code_of_icon("arrow-line-dow") == 0);
static_assert(lv::detail::code_of_icon("") == 0);
static_assert(lv::detail::code_of_icon("ARROW-LEFT") == 0);

// The names offered for a wrong one: the first line of the table too, and only names at most three
// letters longer or shorter, both edges included.
consteval bool nearest_are(std::string_view wanted, std::initializer_list<std::string_view> names) {
  return std::ranges::equal(lv::detail::nearest_icons(wanted), names);
}
static_assert(nearest_are("acorn", {"acorn", "coin", "alarm", "alien", "anchor"}));
static_assert(nearest_are("x", {"x", "at", "axe", "pi", "bag"}));
static_assert(nearest_are("xxxxx", {"axe", "exam", "taxi", "acorn", "alarm"}));
static_assert(nearest_are("union", {"union", "unite", "alien", "coin", "function"}));
static_assert(lv::detail::nearest_icons("the-gear-icon-at-the-top-right-corner").empty());

// The characters of a font of several icons, in the order named.
static_assert(lv::glyphs<"arrow-left", "check"> == "\xee\x81\x98\xee\x86\x82"sv);
static_assert(lv::glyphs<"gear"> == std::string_view{lv::glyph<"gear">});

// The text of an icon is what a label shows.
constexpr auto ui = lv::label({.text = lv::glyph<"gear">});
constexpr auto part = lv::icon<"arrow-left">({.size = {24 * lv::units::px, 24 * lv::units::px}});
constexpr auto bare = lv::icon<"check">();

// A code point below 0x800 has two bytes (no icon is there; the encoder is the same for any text).
static_assert(lv::detail::utf8_of(0xb0) == "\xc2\xb0"sv);
static_assert(lv::detail::utf8_of(0x7ff) == "\xdf\xbf"sv);
static_assert(lv::detail::utf8_of(0x800) == "\xe0\xa0\x80"sv);
static_assert(lv::detail::utf8_of(0xffff) == "\xef\xbf\xbf"sv);

}  // namespace

constexpr auto both = lv::row(lv::icon<"arrow-left">({.id = "back"}), lv::icon<"gear">({.id = "gear"}));

int main() {
  check(std::strlen(lv::glyph<"gear">) == 3);
  lv_init();
  lv_test_display_create(320, 240);
  {
    auto root = lv::mount<both>(lv::screen_active());
    check(root.has_value());
    if (root) {
      check(std::string_view{lv::view<"label">{root->get<"back">().raw()}.get<"text">()} == "\xee\x81\x98");
      check(std::string_view{lv::view<"label">{root->get<"gear">().raw()}.get<"text">()} == "\xee\x89\xb0");
    }
  }
  lv_deinit();
  return failures;
}
