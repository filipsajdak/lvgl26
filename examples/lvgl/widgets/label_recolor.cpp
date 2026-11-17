#include "widgets.hpp"

namespace {

// A label that reads the #rrggbb color commands in its text.
// The label is the whole tree and sits on the screen itself: the text is wider
// than the screen, and the scrollbar this gives must be the screen's own.
constexpr auto ui = lv::label(
    {.align = "center", .recolor = true,
     .text = "Write a #ff0000 red# word, a #00a000 green# word, and a #0000ff blue# word."});

}  // namespace

void widgets::label_recolor(lv::obj screen) {
  lv::place<ui>(screen);
}
