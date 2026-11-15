#include "widgets.hpp"

namespace {

using namespace widgets;

// Off, on, and the same two again as switches that cannot be used.
constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::toggle(),
    lv::toggle({.checked = true}),
    lv::toggle({.disabled = true}),
    lv::toggle({.checked = true, .disabled = true})];

}  // namespace

void widgets::switch_state(lv::obj screen) {
  lv::place<ui>(screen);
}
