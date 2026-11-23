#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same three-quarter arc, turned by 45 and by 135 degrees.
constexpr auto turned_45 = lv::arc(
  {
    .bg_end_angle   = 270 * deg,
    .bg_start_angle = 0 * deg,
    .rotation       = 45 * deg,
    .size           = {120 * px, 120 * px},
    .value          = 60,
  },
  lv::label({.align = "center", .text = "rot=45"})
);

constexpr auto turned_135 = lv::arc(
  {
    .bg_end_angle   = 270 * deg,
    .bg_start_angle = 0 * deg,
    .rotation       = 135 * deg,
    .size           = {120 * px, 120 * px},
    .value          = 60,
  },
  lv::label({.align = "center", .text = "rot=135"})
);

}  // namespace

// The screen itself is the row, as in the C example: the gap between the arcs is
// the one the theme gives a screen. A tree has one root, so each arc is its own.
void widgets::arc_rotation(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<turned_45>(screen);
  lv::place<turned_135>(screen);
}
