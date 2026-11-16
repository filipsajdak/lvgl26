#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.caption.text});

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    // Wider than high, so the bar is horizontal by itself.
    lv::bar({.max_value = 100, .min_value = 0, .size = {90 * pct, 20 * px}, .value = {60, false}})[
        lv::label({.align = "center", .style = caption, .text = "Horizontal"})],
    // Vertical because it says so.
    lv::bar({.max_value = 100, .min_value = 0, .orientation = "vertical",
             .size = {20 * px, 130 * px}, .value = {60, false}})[
        lv::label({.align = "center",
                   .style = caption | lv::style({.text_align = "center"}),
                   .text = "V\ne\nr"})]];

}  // namespace

void widgets::bar_orientation(lv::obj screen) {
  lv::place<ui>(screen);
}
