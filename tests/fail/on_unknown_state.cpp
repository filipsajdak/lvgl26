// A state with a typo: the message lists the states.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("knob", "presed", {.bg_color = lv::raw(red)});

int main() {}
