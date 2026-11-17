#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Two arcs that follow a drag at different speeds, in degrees per second.
constexpr auto slow =
    lv::arc({.bg_end_angle = 45 * deg, .bg_start_angle = 135 * deg, .change_rate = 40,
             .size = {120 * px, 120 * px}, .value = 35})[
        lv::label({.align = "center", .text = "rate=40"})];

constexpr auto fast =
    lv::arc({.bg_end_angle = 45 * deg, .bg_start_angle = 135 * deg, .change_rate = 360,
             .size = {120 * px, 120 * px}, .value = 35})[
        lv::label({.align = "center", .text = "rate=360"})];

}  // namespace

// The screen itself is the row, as in the C example: the gap between the arcs is
// the one the theme gives a screen. A tree has one root, so each arc is its own.
void widgets::arc_change_rate(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<slow>(screen);
  lv::place<fast>(screen);
}
