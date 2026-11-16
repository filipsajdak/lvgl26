#include "widgets.hpp"

namespace {

using namespace widgets;

// How often the button was clicked. The label below it follows.
// The C example writes the label's text in its event handler. One difference: built
// a second time, the C example starts at "Clicks: 0" again while its count goes on;
// here the label always shows the count.
constinit lv::state clicks{lv::counter{}};

void count_click() {
  ++clicks;
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.flex_cross_place = "center",
                                         .pad_row = tok.gap.md}})[
    lv::button({.on_clicked = &count_click})[
        lv::label({.align = "center", .text = "Click me"})],
    lv::label({.text = lv::bind(clicks, "Clicks: {}")})];

}  // namespace

void widgets::button_event(lv::obj screen) {
  lv::place<ui>(screen);
}
