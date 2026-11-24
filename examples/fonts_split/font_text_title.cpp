// The only file that draws the font of text.title.
#include "design.hpp"

namespace {

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);
