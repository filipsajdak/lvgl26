#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A button as it is, one that stays checked, and one that cannot be used.
constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::button({.size = {160 * px, 40 * px}}, lv::label({.align = "center", .text = "Normal"})),
  lv::button(
    {
      .checkable = true,
      .checked   = true,
      .size      = {160 * px, 40 * px},
    },
    lv::label({.align = "center", .text = "Checked"})
  ),
  lv::button(
    {.disabled = true, .size = {160 * px, 40 * px}},
    lv::label({.align = "center", .text = "Disabled"})
  )
);

}  // namespace

void widgets::button_states(lv::obj screen) { lv::place<ui>(screen); }
