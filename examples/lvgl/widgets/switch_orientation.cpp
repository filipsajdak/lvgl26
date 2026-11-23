#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same switch lying down and standing up, both switched on.
constexpr auto lying = lv::toggle({
  .checked     = true,
  .orientation = "horizontal",
  .size        = {60 * px, 30 * px},
});

constexpr auto standing = lv::toggle({
  .checked     = true,
  .orientation = "vertical",
  .size        = {30 * px, 60 * px},
});

}  // namespace

// The screen itself is the row, as in the C example: the gap between the switches
// is the one the theme gives a screen. A tree has one root, so each switch is its own.
void widgets::switch_orientation(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out | lv::style({.flex_cross_place = "center"})});
  lv::place<lying>(screen);
  lv::place<standing>(screen);
}
