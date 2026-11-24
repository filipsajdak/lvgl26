// The only file that draws the font of text.numeral: the digits and the colon of a clock.
#include "design.hpp"

namespace {

constexpr unsigned char condensed_ttf[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
constexpr auto type = lv::typeface({{"Barlow Condensed", 600, condensed_ttf, "0123456789:"}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.numeral"> = lv::draw<"text.numeral">(type);
