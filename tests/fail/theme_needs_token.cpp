// A theme holds styles, so its values are design tokens too.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr auto mine = lv::theme({.button = {.style = {.radius = 8 * px}}});

int main() {}
