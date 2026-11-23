#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Six scales, one for each way to lay a scale out: ticks above, ticks below, a ring
// with ticks outside, a ring with ticks inside, and two upright ones.
constexpr auto ui = lv::group(
  lv::widget<"scale">({
    .align            = "left_mid",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "horizontal_top",
    .size             = {120 * px, 30 * px},
    .total_tick_count = 11,
    .x                = 20 * px,
    .y                = 20 * px,
  }),
  lv::widget<"scale">({
    .align            = "left_mid",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "horizontal_bottom",
    .size             = {120 * px, 30 * px},
    .total_tick_count = 11,
    .x                = 20 * px,
    .y                = -20 * px,
  }),
  lv::widget<"scale">({
    .align            = "center",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "round_outer",
    .size             = {120 * px, 120 * px},
    .total_tick_count = 11,
  }),
  lv::widget<"scale">({
    .align            = "center",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "round_inner",
    .size             = {90 * px, 90 * px},
    .total_tick_count = 11,
  }),
  lv::widget<"scale">({
    .align            = "right_mid",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "vertical_left",
    .size             = {30 * px, 160 * px},
    .total_tick_count = 11,
    .x                = -20 * px,
  }),
  lv::widget<"scale">({
    .align            = "right_mid",
    .label_show       = true,
    .major_tick_every = 2,
    .max_value        = 100,
    .min_value        = 0,
    .mode             = "vertical_right",
    .size             = {30 * px, 160 * px},
    .total_tick_count = 11,
    .x                = -60 * px,
  })
);

}  // namespace

void widgets::scale_modes(lv::obj screen) { lv::place<ui>(screen); }
