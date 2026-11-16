#include "widgets.hpp"

namespace {

using namespace widgets;

// A label with newline characters in its text, and a plain one to compare.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::label({.text = "First line\nSecond line\n\nFourth line"}),
    lv::label({.text = "Just a normal single line text."})];

}  // namespace

void widgets::label_set_text_newline(lv::obj screen) {
  lv::place<ui>(screen);
}
