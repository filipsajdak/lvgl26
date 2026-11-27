// The characters of a drawn font beyond the default set: lv::ascii_plus for a few, lv::latin1 for the accents of
// Latin-1. A picture shows a price with the euro sign and a line with the middle dot.
//
//   font_extra <reference.png>
// editor-check: skip, the test reads the glyphs of a font, which the stubs do not declare

#include <lv/font.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

using namespace lv::units;
using namespace std::string_view_literals;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

// The sets as text: the default 95 first, then what is added, each character once.
constexpr std::string_view latin1_text = lv::latin1;
static_assert(latin1_text.size() == lv::detail::ascii.size() + 2 * 96);
static_assert(latin1_text.starts_with(lv::detail::ascii));
static_assert(latin1_text.substr(lv::detail::ascii.size(), 2) == " "sv);
static_assert(latin1_text.ends_with("ÿ"sv));
static_assert(lv::latin1_plus<""> == latin1_text);
static_assert(lv::latin1_plus<"€">.size() == latin1_text.size() + 3);
static_assert(lv::latin1_plus<"€">.starts_with(lv::latin1));
static_assert(lv::detail::characters_of(latin1_text).size() == 95 + 96);
// Latin-1 stops at U+00FF: the Polish ł (U+0142) and the euro sign are not in it.
static_assert(lv::detail::characters_of(latin1_text).back() == 0xff);
static_assert(lv::detail::characters_of(lv::latin1_plus<"€·">).size() == 95 + 96 + 1);  // the dot is in Latin-1

// The font of the price and of the strength: the default set, the euro sign, the middle dot and three letters.
constexpr const lv_font_t* text_font = lv::font<barlow_ttf>(20 * px, lv::ascii_plus<"€·éÑú">);

// Latin-1 whole, at a small size to keep the build short.
constexpr const lv_font_t* accents = lv::font<barlow_ttf>(10 * px, lv::latin1);

constexpr auto ui = lv::column(
    {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
    lv::label({.text = "2.20 €", .style = {.text_font = lv::raw(text_font)}}),
    lv::label({.text = "4 of 5 · Strong", .style = {.text_font = lv::raw(text_font)}}),
    lv::label({.text = "Café Ñandú", .style = {.text_font = lv::raw(text_font)}}));

lv_font_glyph_dsc_t glyph_of(const lv_font_t* font, std::uint32_t character) {
  lv_font_glyph_dsc_t dsc{};
  lv_font_get_glyph_dsc(font, &dsc, character, 0);
  return dsc;
}

void the_sets_hold_their_glyphs() {
  for (std::uint32_t c : {0x20ACu, 0xB7u, 0xE9u, 0xD1u, 0xFAu}) check(!glyph_of(text_font, c).is_placeholder);
  check(glyph_of(text_font, 0xE8).is_placeholder);  // è was not named
  for (std::uint32_t c = 0xa0; c <= 0xff; ++c) check(!glyph_of(accents, c).is_placeholder);
  for (std::uint32_t c = ' '; c <= '~'; ++c) check(!glyph_of(accents, c).is_placeholder);
  check(glyph_of(accents, 0x20ac).is_placeholder);  // the euro sign is not in Latin-1
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: font_extra <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  the_sets_hold_their_glyphs();
  {
    auto root = lv::mount<ui>(lv::screen_active());
    check(root.has_value());
    if (root) check(lv_test_screenshot_compare(argv[1]) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  }
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
