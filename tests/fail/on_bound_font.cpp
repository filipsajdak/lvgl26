// A layer is a constant; a font that is drawn on the device is not.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constinit lv::scaled_font title{barlow_ttf, lv::raw(20 * px)};
constexpr auto s = lv::on("pressed", {.text_font = lv::bind(title)});

int main() {}
