#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same LED dim, half lit and fully lit. Brightness goes from 0 to 255.
constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::widget<"led">({.brightness = 60, .color = tok.led.orange, .size = {40 * px, 40 * px}}),
    lv::widget<"led">({.brightness = 150, .color = tok.led.orange, .size = {40 * px, 40 * px}}),
    lv::widget<"led">({.brightness = 255, .color = tok.led.orange, .size = {40 * px, 40 * px}})];

}  // namespace

void widgets::led_brightness(lv::obj screen) {
  lv::place<ui>(screen);
}
