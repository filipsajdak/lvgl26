#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same months for both rollers; only the count of visible rows differs.
constexpr auto months = lv::option_list{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul"};

constexpr auto three_rows = lv::roller({
  .options           = {months, "normal"},
  .selected          = {0, false},
  .visible_row_count = 3,
  .width             = 120 * px,
});

constexpr auto five_rows = lv::roller({
  .options           = {months, "normal"},
  .selected          = {3, false},
  .visible_row_count = 5,
  .width             = 120 * px,
});

// The rollers spread along the row, each in the middle of its height.
constexpr auto row_layout = spread_out | lv::style({.flex_cross_place = "center"});

}  // namespace

// The screen itself is the row, as in the C example: the gap between the rollers is the one
// the theme gives a screen. A tree has one root, so each roller is its own.
void widgets::roller_visible_rows(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = row_layout});
  lv::place<three_rows>(screen);
  lv::place<five_rows>(screen);
}
