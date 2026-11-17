// One name that is neither a part nor a state: the message lists both.
#include <lv/lv.hpp>

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr auto s = lv::on("knop", {.bg_color = lv::raw(red)});

int main() {}
