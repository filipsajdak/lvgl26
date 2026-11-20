// The same in a theme: a layer goes in .style.
#include <lv/lv.hpp>
#include <lv/theme.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto mine = lv::theme({.slider = {.style_knob = lv::on("pressed", {.bg_color = lv::raw(red)})}});

int main() {}
