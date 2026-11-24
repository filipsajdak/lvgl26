// The only file that draws Signal's font of text.numeral-m: the characters of a reading: the digits, the point, the colon, the minus, the percent sign, the degree sign and the space.
#include <lv/signal_type_design.hpp>

namespace {

constexpr unsigned char file[] = {
#embed "fonts/BarlowCondensed-Bold.ttf"
};
constexpr auto type = lv::typeface({{"Barlow Condensed", 700, file, "0123456789.:-%\xC2\xB0 "}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.numeral-m", lv::signal::type_design> =
    lv::draw<"text.numeral-m", lv::signal::type_design>(type);
