// The only file that draws Signal's font of text.title: the characters from the space to the tilde.
#include <lv/signal_type_design.hpp>

namespace {

constexpr unsigned char file[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
constexpr auto type = lv::typeface({{"Barlow Condensed", 600, file}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.title", lv::signal::type_design> =
    lv::draw<"text.title", lv::signal::type_design>(type);
