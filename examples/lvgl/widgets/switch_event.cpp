#include "widgets.hpp"

namespace {

using namespace widgets;

// Whether the switch is on, and what the label says about it.
// The C example reads the switch in an event handler and writes the label there.
// One difference: built a second time, the C example starts switched off; here the
// switch shows the state as it is.
constinit lv::state<bool> on{false};
constinit lv::state<const char*> status{"State: OFF"};

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = {.flex_cross_place = "center", .pad_row = tok.gap.md},
  },
  lv::toggle({.checked = lv::bind(on)}),  // follows, and writes back when toggled
  lv::label({.text = lv::bind(status)})
);

}  // namespace

void widgets::switch_event(lv::obj screen) {
  // Once, however often the example is built: the text follows the switch from now on.
  static const auto following = on.observe([](bool now) {
    status = now ? "State: ON" : "State: OFF";
  });
  if (following == nullptr) return;  // LVGL had no memory for the observer
  lv::place<ui>(screen);
}
