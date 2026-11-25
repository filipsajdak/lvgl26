#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A scale from 0 to 100, and one from -50 to 50 with the zero in the middle.
constexpr auto percent = lv::widget<"scale">({
  .label_show       = true,
  .major_tick_every = 2,
  .max_value        = 100,
  .min_value        = 0,
  .mode             = "horizontal_bottom",
  .size             = {260 * px, 60 * px},
  .total_tick_count = 11,
});

constexpr auto centred = lv::widget<"scale">({
  .label_show       = true,
  .major_tick_every = 2,
  .max_value        = 50,
  .min_value        = -50,
  .mode             = "horizontal_bottom",
  .size             = {260 * px, 60 * px},
  .total_tick_count = 11,
});

}  // namespace

// The screen itself is the column, as in the C example: the gap between the scales is
// the one the theme gives a screen. A tree has one root, so each scale is its own.
void widgets::scale_range(lv::obj screen) {
  screen.set(
    {.flex_flow = "column",
     .style     = lv::style({
           .flex_cross_place = "center",
           .flex_main_place  = "space_evenly",
           .flex_track_place = "center",
     })}
  );
  lv::place<percent>(screen);
  lv::place<centred>(screen);
}
