#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Longer and thicker major ticks.
constexpr auto major = lv::style({.length = tok.scale.major_length,
                                  .line_width = tok.scale.major_width});

// The same scale with 5, 11 and 21 ticks: the more ticks, the more minor ones between the majors.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = lv::style({.flex_main_place = "space_evenly",
                                                   .flex_track_place = "center",
                                                   .pad_row = tok.scale.row_gap})})[
    lv::widget<"scale">({.label_show = true, .major_tick_every = 2, .max_value = 100, .min_value = 0,
               .mode = "horizontal_bottom", .size = {260 * px, 30 * px},
               .style_indicator = major, .total_tick_count = 5}),
    lv::widget<"scale">({.label_show = true, .major_tick_every = 5, .max_value = 100, .min_value = 0,
               .mode = "horizontal_bottom", .size = {260 * px, 30 * px},
               .style_indicator = major, .total_tick_count = 11}),
    lv::widget<"scale">({.label_show = true, .major_tick_every = 4, .max_value = 100, .min_value = 0,
               .mode = "horizontal_bottom", .size = {260 * px, 30 * px},
               .style_indicator = major, .total_tick_count = 21})];

}  // namespace

void widgets::scale_ticks(lv::obj screen) {
  lv::place<ui>(screen);
}
