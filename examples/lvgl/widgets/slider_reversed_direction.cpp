#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.caption.text});

// The same value on a slider that runs from 0 to 100, and on one that runs from
// 100 down to 0: a larger minimum than maximum turns the direction around.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::slider({.max_value = 100, .min_value = 0, .size = {90 * pct, 20 * px},
                .value = {30, false}})[
        lv::label({.align = "center", .style = caption, .text = "0..100"})],
    lv::slider({.max_value = 0, .min_value = 100, .size = {90 * pct, 20 * px},
                .value = {30, false}})[
        lv::label({.align = "center", .style = caption, .text = "100..0"})]];

}  // namespace

void widgets::slider_reversed_direction(lv::obj screen) {
  lv::place<ui>(screen);
}
