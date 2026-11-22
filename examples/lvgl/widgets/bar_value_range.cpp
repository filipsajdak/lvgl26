#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto caption = lv::style({.text_color = tok.caption.text});

// The usual range from 0 to 100, and one that does not start at zero.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::bar({.max_value = 100, .min_value = 0, .size = {90 * pct, 20 * px}, .value = {30, false}})[
        lv::label({.align = "center", .text = "0..100", .style = caption})],
    lv::bar({.max_value = 80, .min_value = -20, .size = {90 * pct, 20 * px}, .value = {35, false}})[
        lv::label({.align = "center", .text = "-20..80", .style = caption})]];

}  // namespace

void widgets::bar_value_range(lv::obj screen) {
  lv::place<ui>(screen);
}
