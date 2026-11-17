// A layer names its own part and state, so it goes in .style, not in .style_knob.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto ui = lv::slider({.style_knob = lv::on("pressed", {.bg_color = lv::raw(red)})});

void f(lv::obj screen) { lv::mount<ui>(screen); }
