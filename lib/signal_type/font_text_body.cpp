// The only file that draws Signal's font of text.body: the characters from the space to the tilde.
#include <lv/signal_type_design.hpp>

namespace {

constexpr unsigned char file[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, file}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.body", lv::signal::type_design> =
    lv::draw<"text.body", lv::signal::type_design>(type);
