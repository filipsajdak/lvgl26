#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A thin frame shows how wide each label is.
constexpr auto framed = lv::style({.border_width = tok.label.frame_width});

// The same two lines, aligned left, in the middle and right.
constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::label({.style = framed | lv::style({.text_align = LV_TEXT_ALIGN_LEFT}),
               .text = "Left aligned\nSecond line", .width = 220 * px}),
    lv::label({.style = framed | lv::style({.text_align = LV_TEXT_ALIGN_CENTER}),
               .text = "Centered\nSecond line", .width = 220 * px}),
    lv::label({.style = framed | lv::style({.text_align = LV_TEXT_ALIGN_RIGHT}),
               .text = "Right aligned\nSecond line", .width = 220 * px})];

}  // namespace

void widgets::label_text_align(lv::obj screen) {
  lv::place<ui>(screen);
}
