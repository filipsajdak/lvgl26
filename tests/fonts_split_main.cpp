// Fonts drawn in files of their own: this file names them and draws nothing. The fonts that it
// gets are the ones the compiler draws in place, and one object is shared by every use.
#include <lv/font.hpp>
#include <lv/look.hpp>
#include <lv/theme.hpp>

#include "fonts_split.hpp"

#include "check.hpp"

namespace {

constexpr auto label_font = lv::font_of<"text.label">;
constexpr auto small_font = lv::font_of<"text.small">;

constexpr auto tile = lv::label({.text = "12:30", .style = {.text_font = label_font}});
constexpr auto theme = lv::theme({.label = {.style = {.text_font = small_font}}});

// The same file, size and characters, drawn in this file.
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto in_place = lv::typeface({{"Barlow", 500, barlow}});
constexpr auto here = in_place(split_project::tok.text.label);

void the_name_is_the_object() {
  check(*label_font == &lv::font_object<"text.label">);
  check(*small_font == &lv::font_object<"text.small">);
  check(&lv::font_object<"text.label"> != &lv::font_object<"text.small">);
}

void the_fonts_are_the_ones_drawn_in_place() {
  const lv_font_t* there = *label_font;
  check(there->line_height == (*here)->line_height);
  check(there->base_line == (*here)->base_line);
  for (const char letter : {'H', 'g', '1'}) {
    lv_font_glyph_dsc_t a{}, b{};
    check(lv_font_get_glyph_dsc(there, &a, static_cast<std::uint32_t>(letter), 0));
    check(lv_font_get_glyph_dsc(*here, &b, static_cast<std::uint32_t>(letter), 0));
    check(a.adv_w == b.adv_w && a.box_w == b.box_w && a.box_h == b.box_h && a.ofs_x == b.ofs_x && a.ofs_y == b.ofs_y);
  }
}

void a_font_with_fewer_characters_lacks_the_others() {
  lv_font_glyph_dsc_t g{};
  check(lv_font_get_glyph_dsc(*small_font, &g, '7', 0));
  check(!lv_font_get_glyph_dsc(*small_font, &g, 'Z', 0));
}

void a_label_takes_the_font() {
  lv::application app;
  constinit static lv::frame<320, 240> frame;
  lv::display display{app, frame};
  auto shown = lv::mount<tile>(display.screen());
  check(shown.has_value());
  if (!shown) return;
  check(lv_obj_get_style_text_font(shown->root().raw(), LV_PART_MAIN) == *label_font);
}

}  // namespace

int main() {
  the_name_is_the_object();
  the_fonts_are_the_ones_drawn_in_place();
  a_font_with_fewer_characters_lacks_the_others();
  a_label_takes_the_font();
  (void)theme;
  return failures;
}
