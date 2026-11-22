// A font in a style comes from a typography token. One that no token names is lv::raw(...).
#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr const lv_font_t* odd = lv::font<barlow_ttf>(18 * px);
constexpr auto ui = lv::label({.text = "x", .style = {.text_font = odd}});

int main() {}
