// Only a font property follows a font.
#include <lv/lv.hpp>
#include <lv/scaled_font.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constinit lv::scaled_font title{barlow_ttf, lv::raw(20 * px)};
constexpr auto ui = lv::label({.style = {.radius = lv::bind(title)}});

int main() {}
