#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A slice is the background part of an arc, drawn so wide that it reaches the
// center. All slices lie on top of each other in the middle of the screen.
constexpr auto slice = lv::style({
  .align       = "center",
  .arc_rounded = false,
  .arc_width   = tok.arc.pie.width,
  .height      = tok.arc.pie.size,
  .width       = tok.arc.pie.size,
});

// The indicator and the knob of a slice are not shown.
constexpr auto unseen_arc  = lv::style({.arc_opa = tok.opacity.none});
constexpr auto unseen_fill = lv::style({.bg_opa = tok.opacity.none});

// Five slices of different sizes. Angles go clockwise from 3 o'clock; the first
// slice starts at 12 o'clock (270 degrees).
constexpr auto ui = lv::group(
  lv::arc({
    .bg_end_angle    = 325 * deg,
    .bg_start_angle  = 270 * deg,
    .clickable       = false,
    .style           = slice | lv::style({.arc_color = tok.arc.pie.slice_a}),
    .style_indicator = unseen_arc,
    .style_knob      = unseen_fill,
  }),
  lv::arc({
    .bg_end_angle    = 38 * deg,
    .bg_start_angle  = 325 * deg,
    .clickable       = false,
    .style           = slice | lv::style({.arc_color = tok.arc.pie.slice_b}),
    .style_indicator = unseen_arc,
    .style_knob      = unseen_fill,
  }),
  lv::arc({
    .bg_end_angle    = 162 * deg,
    .bg_start_angle  = 38 * deg,
    .clickable       = false,
    .style           = slice | lv::style({.arc_color = tok.arc.pie.slice_c}),
    .style_indicator = unseen_arc,
    .style_knob      = unseen_fill,
  }),
  lv::arc({
    .bg_end_angle    = 190 * deg,
    .bg_start_angle  = 162 * deg,
    .clickable       = false,
    .style           = slice | lv::style({.arc_color = tok.arc.pie.slice_d}),
    .style_indicator = unseen_arc,
    .style_knob      = unseen_fill,
  }),
  lv::arc({
    .bg_end_angle    = 270 * deg,
    .bg_start_angle  = 190 * deg,
    .clickable       = false,
    .style           = slice | lv::style({.arc_color = tok.arc.pie.slice_e}),
    .style_indicator = unseen_arc,
    .style_knob      = unseen_fill,
  })
);

}  // namespace

void widgets::arc_pie_chart(lv::obj screen) { lv::place<ui>(screen); }
