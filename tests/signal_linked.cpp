// lv/signal_fonts.hpp and the library lvgl26::signal_type: the same fonts and themes as
// tests/signal_type.cpp, drawn once in the library. The pictures are the same pixels.
//
//   signal_linked <dark reference.png> <day reference.png>

#include <lv/lv.hpp>
#include <lv/signal_fonts.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

using namespace lv::units;

constexpr auto text = lv::signal::type_tokens.text;

// The tokens are Signal's: design/signal/tokens.design.json, "type". `line` is the line
// height in pixels, which the token holds as a multiple of the size.
consteval bool is(const lv::token<lv::typography_token>& t, std::string_view family, int size, int weight, int line,
                  int spacing = 0) {
  return t->font_family == family && t->font_size == size * px && t->font_weight == weight &&
         static_cast<int>(size * t->line_height + 0.5) == line && t->letter_spacing == spacing * px;
}
static_assert(*lv::signal::type_tokens.font.display == "Barlow Condensed" && *lv::signal::type_tokens.font.text == "Barlow");
static_assert(is(text.numeral_2xl, "Barlow Condensed", 208, 700, 176) && is(text.numeral_xl, "Barlow Condensed", 104, 700, 94));
static_assert(is(text.numeral_l, "Barlow Condensed", 72, 700, 66) && is(text.numeral_m, "Barlow Condensed", 40, 700, 40));
// Capitals with wide spacing: 8 percent of the size, in whole pixels.
static_assert(is(text.title, "Barlow Condensed", 24, 600, 28, 2) && is(text.label, "Barlow Condensed", 16, 700, 20, 1));
static_assert(is(text.body, "Barlow", 16, 500, 22) && is(text.caption, "Barlow", 13, 500, 18));

static_assert(!lv::signal::dark_with_type.empty() && !lv::signal::day_with_type.empty());

// What a program names itself: a style of the full typeface, and a numeral. The library drew them.
constexpr auto title = lv::font_of<"text.title", lv::signal::type_design>;
constexpr auto reading = lv::font_of<"text.numeral-m", lv::signal::type_design>;
// The fonts of the themes, named again: the same object. Compared at run time, since the
// address of an object that another file defines is no value for a constant expression to read.
constexpr auto body_again = lv::font_of<"text.body", lv::signal::type_design>;
constexpr auto label_again = lv::font_of<"text.label", lv::signal::type_design>;

constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "TANK 2 LEVEL", .style = {.text_font = title}}),
    lv::label({.id = "reading", .text = "72.5 %", .style = {.text_font = reading}}),
    lv::label({.id = "body", .text = "Refill below 20 percent."}),
    lv::button({.id = "start"})[lv::label({.id = "word", .text = "START"})]];

constinit lv::frame<320, 240> frame;

const lv_font_t* font_of(lv_obj_t* o) { return lv_obj_get_style_text_font(o, LV_PART_MAIN); }

std::uint16_t width_of(const lv_font_t* font, std::uint32_t character) { return lv_font_get_glyph_width(font, character, 0); }

bool has(const lv_font_t* font, std::uint32_t character) {
  lv_font_glyph_dsc_t dsc{};
  return lv_font_get_glyph_dsc(font, &dsc, character, 0) && !dsc.is_placeholder;
}

// Each font is the file, the size and the characters that its token and its typeface name.
void fonts_are_the_tokens() {
  const lv_font_t* body = *lv::signal::body_font;
  const lv_font_t* label = *lv::signal::label_font;
  check(body == *body_again && label == *label_again && body != label);
  // Barlow and Barlow Condensed: 1000 units to the em, ascender 1000, descender -200.
  check(body->line_height == 20 && label->line_height == 20);
  check((*title)->line_height == 29 && (*reading)->line_height == 48);
  // An M at 16 px: 11 px wide in Barlow Medium, 9 px in Barlow Condensed Bold.
  check(width_of(body, 'M') == 11 && width_of(label, 'M') == 9);
  // The same letter at 24 px in Barlow Condensed SemiBold.
  check(width_of(*title, 'M') == 13);
  check(has(body, ' ') && has(body, '~') && has(label, ' ') && has(label, '~') && has(*title, '~'));

  // A numeral font holds sixteen characters: the ten digits and these six, the degree
  // sign among them.
  for (std::uint32_t digit = '0'; digit <= '9'; ++digit) check(has(*reading, digit));
  for (const std::uint32_t character : {0x2eu, 0x3au, 0x2du, 0x25u, 0xb0u, 0x20u}) check(has(*reading, character));
  check(static_cast<const lv_font_fmt_txt_dsc_t*>((*reading)->dsc)->cmaps[0].list_length == 16);
  check(!has(*reading, 'A') && !has(*reading, '/') && !has(*reading, ','));
  check(width_of(*reading, '7') == 16);  // at 40 px
}

// The fonts a theme gives: `body` to all text, `label` to the text of a button.
void sets_text(lv::theme_styles theme, const char* reference) {
  lv::application app;
  lv::display display{app, frame, theme};
  lv_obj_t* screen = display.screen().raw();
  const lv_font_t* body = *lv::signal::body_font;
  const lv_font_t* label = *lv::signal::label_font;

  // Text is inherited: a label takes the font of the screen through any box it is in,
  // and the font of a button through a box in that button.
  lv_obj_t* box = lv_obj_create(screen);
  check(font_of(screen) == body && font_of(box) == body);
  check(font_of(lv_label_create(screen)) == body && font_of(lv_label_create(box)) == body);
  lv_obj_t* button = lv_button_create(box);
  check(font_of(button) == label && font_of(lv_label_create(button)) == label);
  check(font_of(lv_label_create(lv_obj_create(button))) == label);
  // The text of a button is spaced as `label` says; other text is not.
  check(lv_obj_get_style_text_letter_space(button, LV_PART_MAIN) == 1);
  check(lv_obj_get_style_text_letter_space(lv_label_create(button), LV_PART_MAIN) == 1);
  check(lv_obj_get_style_text_letter_space(lv_label_create(screen), LV_PART_MAIN) == 0);
  // A widget that the theme leaves to LVGL has the font of the screen too.
  check(font_of(lv_checkbox_create(screen)) == body);
  lv_obj_clean(screen);

  auto root = lv::mount<ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  check(font_of(root->get<"body">().raw()) == body);
  check(font_of(root->get<"start">().raw()) == label && font_of(root->get<"word">().raw()) == label);
  // A node has the last word.
  check(font_of(root->get<"title">().raw()) == *title && font_of(root->get<"reading">().raw()) == *reading);
  check(lv_test_screenshot_compare(reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  // A line of text is as high as its font, once the screen is laid out.
  check(lv_obj_get_height(root->get<"body">().raw()) == 20 && lv_obj_get_height(root->get<"word">().raw()) == 20);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::printf("usage: signal_linked <dark reference.png> <day reference.png>\n");
    return 2;
  }
  fonts_are_the_tokens();
  sets_text(lv::signal::dark_with_type, argv[1]);
  sets_text(lv::signal::day_with_type, argv[2]);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
