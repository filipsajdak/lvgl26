// A part with a typo: the message lists the parts.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("knop", "pressed", {.bg_color = lv::raw(red)});

int main() {}
