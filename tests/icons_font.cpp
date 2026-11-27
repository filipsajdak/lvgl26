// The Phosphor Bold file that lv/icons.hpp gives, drawn into a font of the icons a program names.
#include <lv/font.hpp>
#include <lv/icons.hpp>
#include <lv/icons_font.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

namespace {

using namespace lv::units;

constexpr auto icon_type = lv::typeface({{"Phosphor", 700, lv::icons::phosphor_bold_ttf, lv::glyphs<"arrow-left", "check", "gear">}});

// The font of the three icons at 24 px: three glyphs, and the characters are those of the icons.
constexpr const lv_font_t* font24 = lv::font<lv::icons::phosphor_bold_ttf>(24 * px, lv::glyphs<"arrow-left", "check", "gear">);
static_assert(font24->line_height > 0);

}  // namespace

int main() {
  lv_font_glyph_dsc_t dsc{};
  check(lv_font_get_glyph_dsc(font24, &dsc, 0xe058, 0));  // arrow-left
  check(dsc.adv_w > 0 && dsc.box_w > 0 && dsc.box_h > 0);
  check(lv_font_get_glyph_dsc(font24, &dsc, 0xe182, 0));  // check
  check(lv_font_get_glyph_dsc(font24, &dsc, 0xe270, 0));  // gear
  check(!lv_font_get_glyph_dsc(font24, &dsc, 0xe3d4, 0));  // plus: not named, not in the font
  return failures;
}
