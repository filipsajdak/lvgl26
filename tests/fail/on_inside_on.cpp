// A layer inside a layer would need two parts or two states.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("knob", lv::on("pressed", {.bg_color = lv::raw(red)}));

int main() {}
