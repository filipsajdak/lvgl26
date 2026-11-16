#include "widgets.hpp"

namespace {

using namespace widgets;

// Unticked, ticked, and the same two again as boxes that cannot be used.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::checkbox({.text = "Unchecked"}),
    lv::checkbox({.checked = true, .text = "Checked"}),
    lv::checkbox({.disabled = true, .text = "Disabled"}),
    lv::checkbox({.checked = true, .disabled = true, .text = "Disabled + Checked"})];

}  // namespace

void widgets::checkbox_states(lv::obj screen) {
  lv::place<ui>(screen);
}
