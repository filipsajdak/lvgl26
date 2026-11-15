#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.bar.caption});

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    // Wider than high, so the bar is horizontal by itself.
    lv::bar({.max_value = 100, .min_value = 0, .size = {90 * pct, 20 * px}, .value = {60, false}})[
        lv::label({.align = LV_ALIGN_CENTER, .style = caption, .text = "Horizontal"})],
    // Vertical because it says so.
    lv::bar({.max_value = 100, .min_value = 0, .orientation = LV_BAR_ORIENTATION_VERTICAL,
             .size = {20 * px, 130 * px}, .value = {60, false}})[
        lv::label({.align = LV_ALIGN_CENTER,
                   .style = caption | lv::style({.text_align = LV_TEXT_ALIGN_CENTER}),
                   .text = "V\ne\nr"})]];

}  // namespace

void widgets::bar_orientation(lv::obj screen) {
  lv::place<ui>(screen);
}
