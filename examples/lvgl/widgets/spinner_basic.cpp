// Ports lv_example_spinner_basic. Re-ordered: lv_obj_set_size runs after
// lv_spinner_set_anim_duration and lv_spinner_set_arc_sweep.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The size of each spinner in the C example.
constexpr auto side = 80 * px;

// A long arc at a medium speed.
constexpr auto long_arc = lv::spinner({
  .anim_duration = 1500 * ms,
  .arc_sweep     = 270 * deg,
  .size          = {side, side},
});

// A short arc, faster.
constexpr auto short_arc = lv::spinner({
  .anim_duration = 600 * ms,
  .arc_sweep     = 180 * deg,
  .size          = {side, side},
});

}  // namespace

// The screen itself is the row, as in the C example: the gap between the spinners is the one
// the theme gives a screen. A tree has one root, so each spinner is its own.
void widgets::spinner_basic(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<long_arc>(screen);
  lv::place<short_arc>(screen);
}
