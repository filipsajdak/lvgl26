// The only file that draws the font of text.small: sixteen characters of the same Barlow.
#include "fonts_split.hpp"

namespace {

constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow, "0123456789:. abc"}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.small"> = lv::draw<"text.small">(type);
