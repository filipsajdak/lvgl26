#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same days for both rollers. A roller takes the list and its mode together.
//! [roller]
constexpr auto normal = lv::roller({.options = {{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"}, "normal"},
                                    .selected = {0, false}, .visible_row_count = 3, .width = 120 * px});

constexpr auto infinite = lv::roller({.options = {{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"}, "infinite"},
                                      .selected = {2, false}, .visible_row_count = 3, .width = 120 * px});
//! [roller]

}  // namespace

// The screen itself is the row, as in the C example: the gap between the rollers is
// the one the theme gives a screen. A tree has one root, so each roller is its own.
void widgets::roller_options(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<normal>(screen);
  lv::place<infinite>(screen);
}
