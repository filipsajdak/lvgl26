#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same value, 123, with two formats: three digits, and five digits with
// the decimal point two places from the right.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = lv::style({.flex_main_place = "center",
                                                   .flex_track_place = "center",
                                                   .pad_row = tok.gap.lg})})[
    lv::widget<"spinbox">({.dec_point_pos = 0, .digit_count = 3, .step = 1, .value = 123,
                 .width = 160 * px}),
    lv::widget<"spinbox">({.dec_point_pos = 2, .digit_count = 5, .step = 1, .value = 123,
                 .width = 160 * px})];

}  // namespace

void widgets::spinbox_format(lv::obj screen) {
  lv::place<ui>(screen);
}
