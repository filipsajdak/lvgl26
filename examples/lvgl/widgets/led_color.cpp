#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A red, a green and a blue LED.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::widget<"led">({.color = tok.led.red, .size = {40 * px, 40 * px}}),
    lv::widget<"led">({.color = tok.led.green, .size = {40 * px, 40 * px}}),
    lv::widget<"led">({.color = tok.led.blue, .size = {40 * px, 40 * px}})];

}  // namespace

void widgets::led_color(lv::obj screen) {
  lv::place<ui>(screen);
}
