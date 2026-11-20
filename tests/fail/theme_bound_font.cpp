// A theme is a constant; a font that is drawn on the device is not.
#include <lv/lv.hpp>
#include <lv/scaled_font.hpp>
#include <lv/theme.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constinit lv::scaled_font title{barlow_ttf, lv::raw(20 * px)};
constexpr auto mine = lv::theme({.label = {.style = {.text_font = lv::bind(title)}}});

int main() {}
