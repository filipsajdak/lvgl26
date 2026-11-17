#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Four dropdowns, each opening its list in another direction.
constexpr auto ui = lv::group[
    lv::dropdown({.align = "top_mid", .options = "Apple\nBanana\nOrange\nMelon", .y = 20 * px}),
    lv::dropdown({.align = "bottom_mid", .dir = "top", .options = "Apple\nBanana\nOrange\nMelon",
                  .y = -20 * px}),
    lv::dropdown({.align = "left_mid", .dir = "right", .options = "Apple\nBanana\nOrange\nMelon",
                  .width = 80 * px, .x = 20 * px}),
    lv::dropdown({.align = "right_mid", .dir = "left", .options = "Apple\nBanana\nOrange\nMelon",
                  .width = 80 * px, .x = -20 * px})];

}  // namespace

void widgets::dropdown_direction(lv::obj screen) {
  lv::place<ui>(screen);
}
