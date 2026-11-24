// The only file that draws the font of text.label.
#include "fonts_split.hpp"

namespace {

constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.label"> = lv::draw<"text.label">(type);
