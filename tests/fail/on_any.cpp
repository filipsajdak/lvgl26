// `any` selects every part or state when LVGL removes a style. A style is added for one.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("any", {.bg_color = lv::raw(red)});

int main() {}
