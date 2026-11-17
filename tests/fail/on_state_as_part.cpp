// The state first: the message says the part comes first.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("pressed", "knob", {.bg_color = lv::raw(red)});

int main() {}
