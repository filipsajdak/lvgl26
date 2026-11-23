#include "widgets.hpp"

#include <mp-units/systems/si.h>

namespace {

using namespace widgets;
using namespace lv::units;
using mp_units::percent;

// The slider's value, and whether it is too high to submit.
constinit lv::state level{50 * percent};
constinit lv::state<bool> too_high{false};

// The C example gives the button a function of the value (lv_obj_bind_int). A
// binding has no function, so a second state holds the result and the button's
// `disabled` follows that.
constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::slider({.value = lv::bind(level), .width = 90 * pct}),
  lv::label({.text = lv::bind(level, "Current: {} (disabled above 80)")}),
  lv::button({.disabled = lv::bind(too_high)}, lv::label({.align = "center", .text = "Submit"}))
);

}  // namespace

void widgets::slider_bind_state(lv::obj screen) {
  // Once, however often the example is built: too_high follows level from now on.
  static const auto following = level.observe([](auto now) {
    too_high = now > 80 * percent;
  });
  if (following == nullptr) return;  // LVGL had no memory for the observer
  lv::place<ui>(screen);
}
