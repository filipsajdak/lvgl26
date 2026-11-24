// The only file that draws Signal's font of text.label: the characters from the space to the tilde.
#include <lv/signal_type_design.hpp>

namespace {

constexpr unsigned char file[] = {
#embed "fonts/BarlowCondensed-Bold.ttf"
};
constexpr auto type = lv::typeface({{"Barlow Condensed", 700, file}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.label", lv::signal::type_design> =
    lv::draw<"text.label", lv::signal::type_design>(type);
