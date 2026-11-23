#include "widgets.hpp"

namespace {

using namespace widgets;

// Whether the box is ticked, and what the label says about it.
// The C example reads the box in an event handler and writes the label there.
// One difference: built a second time, the C example starts unticked; here the box
// shows the state as it is.
constinit lv::state<bool> enabled{false};
constinit lv::state<const char*> status{"Feature: OFF"};

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = {.flex_cross_place = "center", .pad_row = tok.gap.md},
  },
  lv::checkbox({.checked = lv::bind(enabled), .text = "Enable feature"}),
  lv::label({.text = lv::bind(status)})
);

}  // namespace

void widgets::checkbox_event(lv::obj screen) {
  // Once, however often the example is built: the text follows the box from now on.
  static const auto following =
    enabled.observe([](bool now) { status = now ? "Feature: ON" : "Feature: OFF"; });
  if (following == nullptr) return;  // LVGL had no memory for the observer
  lv::place<ui>(screen);
}
