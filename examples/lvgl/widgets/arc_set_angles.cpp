#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The indicator follows a value between 100 and 200.
constexpr auto by_value =
    lv::arc({.bg_end_angle = 45 * deg, .bg_start_angle = 135 * deg, .max_value = 200,
             .min_value = 100, .size = {120 * px, 120 * px}, .value = 145})[
        lv::label({.align = "center", .text = "value"})];

// The indicator is given by its two angles, whatever the value is. No knob.
constexpr auto by_angles =
    lv::arc({.bg_end_angle = 45 * deg, .bg_start_angle = 135 * deg, .end_angle = 330 * deg,
             .size = {120 * px, 120 * px}, .start_angle = 210 * deg,
             .style_knob = {.bg_opa = tok.opacity.none}})[
        lv::label({.align = "center", .text = "angles"})];

}  // namespace

// The screen itself is the row, as in the C example: the gap between the arcs is
// the one the theme gives a screen. A tree has one root, so each arc is its own.
void widgets::arc_set_angles(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<by_value>(screen);
  lv::place<by_angles>(screen);
}
