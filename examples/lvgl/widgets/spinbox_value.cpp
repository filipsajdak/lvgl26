#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A spinbox from 0 to 100 that changes by 3 with each press of an arrow.
constexpr auto ui = lv::group[
    lv::label({.align = "center", .text = "Use the arrows to change the value", .y = -50 * px}),
    lv::widget<"spinbox">({.align = "center", .digit_count = 3, .max_value = 100, .min_value = 0,
                 .step = 3, .value = 25, .width = 160 * px})];

}  // namespace

void widgets::spinbox_value(lv::obj screen) {
  lv::place<ui>(screen);
}
