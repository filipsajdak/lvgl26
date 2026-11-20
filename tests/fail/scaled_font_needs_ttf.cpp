// This file is compiled as if LVGL had no TinyTTF (see tests/CMakeLists.txt).
#include <lv/lv.hpp>
#include <lv/scaled_font.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constinit lv::scaled_font title{barlow_ttf, lv::raw(20 * px)};

constexpr auto ui = lv::label({.style = {.text_font = lv::bind(title)}});

int main() {}
