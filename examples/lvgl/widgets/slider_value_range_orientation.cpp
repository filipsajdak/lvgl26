#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.caption.text});

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    // Horizontal, with a range that does not start at zero.
    lv::slider({.max_value = 80, .min_value = -20, .size = {90 * pct, 20 * px},
                .value = {35, false}})[
        lv::label({.align = "center", .text = "Horizontal", .style = caption})],
    // Vertical because it says so.
    lv::slider({.max_value = 100, .min_value = 0,
                .orientation = "vertical", .size = {20 * px, 130 * px},
                .value = {60, false}})[
        lv::label({.align = "center",
                   .text = "V\ne\nr",
                   .style = caption | lv::style({.text_align = "center"})})]];

}  // namespace

void widgets::slider_value_range_orientation(lv::obj screen) {
  lv::place<ui>(screen);
}
