#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A field for numbers on the top, and the keyboard with the keys of numbers below it.
constexpr auto ui = lv::group[
    lv::textarea({.align = "top_mid", .one_line = true, .placeholder_text = "Type some numbers",
                  .width = 90 * pct, .y = 10 * px}),
    lv::widget<"keyboard">({.align = "bottom_mid", .mode = "number", .size = {100 * pct, 60 * pct}})];

}  // namespace

void widgets::keyboard_modes(lv::obj screen) {
  lv::place<ui>(screen);
}
