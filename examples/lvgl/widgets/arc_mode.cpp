#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same value drawn three ways: from the start, from the end, and from the middle.
constexpr auto normal =
    lv::arc({.mode = LV_ARC_MODE_NORMAL, .size = {95 * px, 95 * px}, .value = 75})[
        lv::label({.align = LV_ALIGN_CENTER, .text = "normal"})];

constexpr auto reverse =
    lv::arc({.mode = LV_ARC_MODE_REVERSE, .size = {95 * px, 95 * px}, .value = 75})[
        lv::label({.align = LV_ALIGN_CENTER, .text = "rev."})];

constexpr auto symmetrical =
    lv::arc({.mode = LV_ARC_MODE_SYMMETRICAL, .size = {95 * px, 95 * px}, .value = 75})[
        lv::label({.align = LV_ALIGN_CENTER, .text = "sym."})];

}  // namespace

// The screen itself is the row, as in the C example: the gap between the arcs is
// the one the theme gives a screen. A tree has one root, so each arc is its own.
void widgets::arc_mode(lv::obj screen) {
  screen.set({.flex_flow = LV_FLEX_FLOW_ROW, .style = spread_out});
  lv::place<normal>(screen);
  lv::place<reverse>(screen);
  lv::place<symmetrical>(screen);
}
