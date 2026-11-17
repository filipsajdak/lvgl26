#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Past 23 the value wraps to 0, and below 0 it wraps to 23.
constexpr auto ui = lv::group[
    lv::label({.align = "center", .text = "Use the arrows to change the value", .y = -50 * px}),
    lv::widget<"spinbox">({.align = "center", .digit_count = 2, .max_value = 23, .min_value = 0,
                 .rollover = true, .step = 1, .value = 22, .width = 160 * px})];

}  // namespace

void widgets::spinbox_rollover(lv::obj screen) {
  lv::place<ui>(screen);
}
