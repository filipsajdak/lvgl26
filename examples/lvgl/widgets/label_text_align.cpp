#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A thin frame shows how wide each label is.
constexpr auto framed = lv::style({.border_width = tok.label.frame_width});

// The same two lines, aligned left, in the middle and right.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::label({.style = framed | lv::style({.text_align = "left"}),
               .text = "Left aligned\nSecond line", .width = 220 * px}),
    lv::label({.style = framed | lv::style({.text_align = "center"}),
               .text = "Centered\nSecond line", .width = 220 * px}),
    lv::label({.style = framed | lv::style({.text_align = "right"}),
               .text = "Right aligned\nSecond line", .width = 220 * px})];

}  // namespace

void widgets::label_text_align(lv::obj screen) {
  lv::place<ui>(screen);
}
