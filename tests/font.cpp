// lv::font and lv::typeface: the compiler makes a constant font from a TrueType file.
// editor-check: skip, the test reads the file of a typeface and the glyphs of a font, which the stubs do not declare
//
//   font <reference.png>

#include <lv/font.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstdio>

namespace {

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char condensed_ttf[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
// Two fonts from LVGL's own sources, for the forms of a file that Barlow does not have:
// Ubuntu finds its glyphs through four-byte offsets, and Montserrat wraps its kerning
// tables in lookups of type 9.
constexpr unsigned char ubuntu_ttf[] = {
#embed "examples/libs/tiny_ttf/Ubuntu-Medium.ttf"
};
constexpr unsigned char montserrat_ttf[] = {
#embed "scripts/generators/built_in_font/Montserrat-Medium.ttf"
};
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

// The test tokens call their family Montserrat; here Barlow stands in for it.
constexpr auto type = lv::typeface({{"Montserrat", 400, barlow_ttf}, {"Montserrat", 700, condensed_ttf}});

// The file of a token is the one of its family and its weight, wherever it is in the list.
constexpr lv::typeface no_files{};
constexpr auto before = lv::typeface({{"Abc", 400, barlow_ttf}, {"Montserrat", 400, condensed_ttf}});
constexpr auto after = lv::typeface({{"Zzz", 400, barlow_ttf}, {"Montserrat", 400, condensed_ttf}});
constexpr auto heavy_first = lv::typeface({{"Montserrat", 700, barlow_ttf}, {"Montserrat", 400, condensed_ttf}});
static_assert(type.file_of(tok.text.body) == 0 && type.file_of(tok.text.title) == 1);
static_assert(before.file_of(tok.text.body) == 1 && after.file_of(tok.text.body) == 1);
static_assert(heavy_first.file_of(tok.text.body) == 1 && heavy_first.file_of(tok.text.title) == 0);

constexpr auto body = type(tok.text.body);    // 14 px, weight 400
constexpr auto title = type(tok.text.title);  // 20 px, weight 700
constexpr const lv_font_t* odd = lv::font<barlow_ttf>(32 * px);
constexpr const lv_font_t* digits = lv::font<condensed_ttf>(64 * px, "0123456789:");
constexpr const lv_font_t* two = lv::font<barlow_ttf>(16 * px, "\xC2\xB0""C9");  // the degree sign, C and 9: not in a row

// A font of blank glyphs only: it has no pixels at all.
constexpr const lv_font_t* blank = lv::font<barlow_ttf>(16 * px, " ");

// A font whose first character kerns with its second.
constexpr const lv_font_t* pair = lv::font<barlow_ttf>(32 * px, "AV");

constexpr const lv_font_t* ubuntu = lv::font<ubuntu_ttf>(24 * px, "AVTo.");
constexpr const lv_font_t* montserrat = lv::font<montserrat_ttf>(24 * px, "AVTo.");

// The same file, size and characters are one font in memory. Checked at run time: under
// a sanitizer GCC 16.2 does not compare two static addresses in a constant expression.
constexpr const lv_font_t* title_again = lv::font<condensed_ttf>(20 * px);

// Barlow: 1000 units to the em, ascender 1000, descender -200.
static_assert(odd->line_height == 39 && odd->base_line == 7);
static_assert((*body)->line_height == 17 && (*body)->base_line == 3);

static_assert(lv::detail::characters_of("ba\xC2\xB0""a") == std::vector<std::uint32_t>{'a', 'b', 0xb0});
static_assert(lv::detail::characters_of("\xE2\x82\xAC") == std::vector<std::uint32_t>{0x20ac});
static_assert(lv::detail::characters_of("\xF0\x9F\x98\x80") == std::vector<std::uint32_t>{0x1f600});
static_assert(lv::detail::hex(0xb0) == "00B0" && lv::detail::hex(0x1f600) == "1F600");

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "body", .text = "The quick brown fox, AVATAR To.", .style = {.text_font = body}}),
    lv::label({.id = "title", .text = "Speed 120 km/h", .style = {.text_font = title}}),
    lv::label({.id = "odd", .text = "Reset {}[]@#", .style = {.text_font = lv::raw(odd)}}),
    lv::label({.id = "digits", .text = "12:45", .style = {.text_font = lv::raw(digits)}})];

// The default characters and two more: the euro sign and the middle dot, as a price and a separator.
constexpr const lv_font_t* priced = lv::font<barlow_ttf>(16 * px, lv::ascii_plus<"\u20AC\u00B7">);
constexpr const lv_font_t* plain = lv::font<barlow_ttf>(16 * px);
constexpr const lv_font_t* reversed = lv::font<barlow_ttf>(16 * px, lv::ascii_plus<"\u00B7\u20AC\u20AC">);
//! [extra]
constexpr auto price_type = lv::typeface({{"Montserrat", 400, barlow_ttf, lv::ascii_plus<"€·">}});
constexpr auto price = price_type(tok.text.body);  // the default characters, and the euro sign and the middle dot
//! [extra]
static_assert(lv::ascii_plus<""> == lv::detail::ascii);
static_assert(lv::ascii_plus<"\u20AC\u00B7">.size() == lv::detail::ascii.size() + 3 + 2);
static_assert(lv::ascii_plus<"\u20AC\u00B7">.starts_with(lv::detail::ascii));
static_assert(lv::detail::invalid_utf8_at("abc") == 3 && lv::detail::invalid_utf8_at("\u20AC") == 3);
static_assert(lv::detail::invalid_utf8_at("a\x80") == 1 && lv::detail::invalid_utf8_at("a\xE2\x82") == 1);
static_assert(lv::detail::invalid_utf8_at("a\xE2\x20\x20") == 2 && lv::detail::invalid_utf8_at("\xF8") == 0);

struct glyph {
  bool found;
  lv_font_glyph_dsc_t dsc;
};

glyph glyph_of(const lv_font_t* font, std::uint32_t character, std::uint32_t next = 0) {
  glyph g{};
  g.found = lv_font_get_glyph_dsc(font, &g.dsc, character, next);
  return g;
}

// The extra characters are in the font beside the default ones; without them the font has none of them.
void holds_extra_characters() {
  check(glyph_of(*price, 0x20ac).found && !glyph_of(*price, 0x20ac).dsc.is_placeholder);
  check(glyph_of(priced, 0x20ac).found && !glyph_of(priced, 0x20ac).dsc.is_placeholder && glyph_of(priced, 0x20ac).dsc.box_w > 0);
  check(glyph_of(priced, 0xb7).found && !glyph_of(priced, 0xb7).dsc.is_placeholder);
  for (std::uint32_t c = ' '; c <= '~'; ++c) check(glyph_of(priced, c).found && !glyph_of(priced, c).dsc.is_placeholder);
  check(glyph_of(plain, 0x20ac).dsc.is_placeholder && glyph_of(plain, 0xb7).dsc.is_placeholder);
  // The order and the repeats of the extras change nothing.
  check(glyph_of(reversed, 0x20ac).dsc.adv_w == glyph_of(priced, 0x20ac).dsc.adv_w);
  check(glyph_of(reversed, 'A').dsc.adv_w == glyph_of(priced, 'A').dsc.adv_w);
  // The characters are one range, until one lies outside it: then the map is a list.
  const auto* plain_map = static_cast<const lv_font_fmt_txt_dsc_t*>(plain->dsc)->cmaps;
  const auto* priced_map = static_cast<const lv_font_fmt_txt_dsc_t*>(priced->dsc)->cmaps;
  check(plain_map->type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY && priced_map->type == LV_FONT_FMT_TXT_CMAP_SPARSE_TINY);
}

void holds_its_characters() {
  check(*title == title_again && *body != *title);
  check(glyph_of(odd, 'A').found && glyph_of(odd, '~').found && glyph_of(odd, ' ').found);
  check(glyph_of(odd, ' ').dsc.box_w == 0 && glyph_of(odd, ' ').dsc.adv_w > 0);
  check(glyph_of(digits, '0').found && glyph_of(digits, ':').found);
  check(glyph_of(digits, 'A').dsc.is_placeholder);  // not in this font
  check(glyph_of(two, 0xb0).found && glyph_of(two, 'C').found && glyph_of(two, '9').found);
  check(glyph_of(two, 'A').dsc.is_placeholder && glyph_of(two, ':').dsc.is_placeholder);
  check(!glyph_of(two, 'C').dsc.is_placeholder);
}

void holds_blank_glyphs_only() {
  const auto space = glyph_of(blank, ' ');
  check(space.found && space.dsc.box_w == 0 && space.dsc.adv_w > 0);
}

void measures_as_the_file_says() {
  // Barlow Medium at 32 px: A advances 20 px and stands 23 px high on the baseline.
  const auto a = glyph_of(odd, 'A').dsc;
  check(a.adv_w == 20 && a.box_w == 20);
  check(a.box_h == 23 && a.ofs_y == 0);
  // A descender goes below the baseline.
  check(glyph_of(odd, 'g').dsc.ofs_y < 0);
  // A composite glyph: the colon is two dots.
  check(glyph_of(odd, ':').dsc.box_h > glyph_of(odd, '.').dsc.box_h);
}

void kerns() {
  const auto* dsc = static_cast<const lv_font_fmt_txt_dsc_t*>(odd->dsc);
  check(static_cast<const lv_font_fmt_txt_kern_pair_t*>(dsc->kern_dsc)->pair_cnt == 748);  // of 95 x 95
  check(lv_font_get_glyph_width(odd, 'A', 0) == 20 && lv_font_get_glyph_width(odd, 'A', 'V') == 19);
  check(lv_font_get_glyph_width(odd, 'T', 0) == 18 && lv_font_get_glyph_width(odd, 'T', 'o') == 16);
  check(lv_font_get_glyph_width(odd, 'H', 'H') == lv_font_get_glyph_width(odd, 'H', 0));
  // The first glyph of a font has its row of pairs too.
  check(lv_font_get_glyph_width(pair, 'A', 0) == 20 && lv_font_get_glyph_width(pair, 'A', 'V') == 19);
}

void reads_other_files() {
  // Ubuntu Medium at 24 px.
  check(glyph_of(ubuntu, 'A').dsc.adv_w == 16 && glyph_of(ubuntu, 'A').dsc.box_h == 17);
  check(glyph_of(ubuntu, '.').dsc.box_w == 5 && glyph_of(ubuntu, '.').dsc.box_h == 5);
  check(lv_font_get_glyph_width(ubuntu, 'A', 'V') == 15 && lv_font_get_glyph_width(ubuntu, 'T', 'o') == 13);
  // Montserrat Medium at 24 px.
  check(glyph_of(montserrat, 'A').dsc.adv_w == 18 && glyph_of(montserrat, 'A').dsc.box_h == 17);
  check(lv_font_get_glyph_width(montserrat, 'A', 'V') == 17);
  check(lv_font_get_glyph_width(montserrat, 'T', 0) == 14 && lv_font_get_glyph_width(montserrat, 'T', 'o') == 13);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: font <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  holds_its_characters();
  holds_extra_characters();
  holds_blank_glyphs_only();
  measures_as_the_file_says();
  kerns();
  reads_other_files();
  {
    auto root = lv::mount<ui>(lv::screen_active());
    check(root.has_value());
    if (root) {
      check(lv_obj_get_style_text_font(root->get<"title">().raw(), LV_PART_MAIN) == *title);
      check(lv_obj_get_style_text_font(root->get<"odd">().raw(), LV_PART_MAIN) == odd);
      check(lv_test_screenshot_compare(argv[1]) == LV_TEST_SCREENSHOT_RESULT_PASSED);
    }
  }
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
