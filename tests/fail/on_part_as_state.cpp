// Two parts: the second name must be a state.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("knob", "indicator", {.bg_color = lv::raw(red)});

int main() {}
