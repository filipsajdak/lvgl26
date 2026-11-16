#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.caption.text});

// A bar filled from its start, one filled from zero in the middle, and one that
// shows a range. The second number of .value says whether the change is animated.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::bar({.max_value = 100, .min_value = 0, .mode = "normal",
             .size = {90 * pct, 20 * px}, .value = {70, false}})[
        lv::label({.align = "center", .style = caption, .text = "normal"})],
    lv::bar({.max_value = 100, .min_value = -100, .mode = "symmetrical",
             .size = {90 * pct, 20 * px}, .value = {45, false}})[
        lv::label({.align = "center", .style = caption, .text = "symmetrical"})],
    lv::bar({.max_value = 100, .min_value = 0, .mode = "range",
             .size = {90 * pct, 20 * px}, .start_value = {25, false}, .value = {80, false}})[
        lv::label({.align = "center", .style = caption, .text = "range"})]];

}  // namespace

void widgets::bar_modes(lv::obj screen) {
  lv::place<ui>(screen);
}
