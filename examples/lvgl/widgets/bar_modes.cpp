#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.bar.caption});

// A bar filled from its start, one filled from zero in the middle, and one that
// shows a range. The second number of .value says whether the change is animated.
constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::bar({.max_value = 100, .min_value = 0, .mode = LV_BAR_MODE_NORMAL,
             .size = {90 * pct, 20 * px}, .value = {70, false}})[
        lv::label({.align = LV_ALIGN_CENTER, .style = caption, .text = "normal"})],
    lv::bar({.max_value = 100, .min_value = -100, .mode = LV_BAR_MODE_SYMMETRICAL,
             .size = {90 * pct, 20 * px}, .value = {45, false}})[
        lv::label({.align = LV_ALIGN_CENTER, .style = caption, .text = "symmetrical"})],
    lv::bar({.max_value = 100, .min_value = 0, .mode = LV_BAR_MODE_RANGE,
             .size = {90 * pct, 20 * px}, .start_value = {25, false}, .value = {80, false}})[
        lv::label({.align = LV_ALIGN_CENTER, .style = caption, .text = "range"})]];

}  // namespace

void widgets::bar_modes(lv::obj screen) {
  lv::place<ui>(screen);
}
