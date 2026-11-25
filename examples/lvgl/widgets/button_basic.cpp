#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A button as large as its label needs, and one with a size of its own.
constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::button(lv::label({.align = "center", .text = "Click me"})),
  lv::button({.size = {160 * px, 48 * px}}, lv::label({.align = "center", .text = "Wide button"}))
);

}  // namespace

void widgets::button_basic(lv::obj screen) { lv::place<ui>(screen); }
