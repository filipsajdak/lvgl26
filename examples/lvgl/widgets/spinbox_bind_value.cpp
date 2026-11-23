#include "widgets.hpp"

#include <algorithm>

namespace {

using namespace widgets;
using namespace lv::units;

// The value that the arc and the spinbox show. The two buttons change it, and so does
// the spinbox. The C example binds all of them to one integer subject limited to 0..100.
// A state has no limits, so the steps below keep the value inside them.
struct amount {
  std::int32_t value;
};
constinit lv::state<amount> shown{{50}};

constexpr std::int32_t lowest  = 0;
constexpr std::int32_t highest = 100;

void step_by(std::int32_t delta) {
  shown.update([delta](amount now) {
    return amount{std::clamp(now.value + delta, lowest, highest)};
  });
}

void lower() { step_by(-1); }

void raise() { step_by(1); }

constexpr auto ui = lv::arc(
  {
    .align = "center",
    .size  = {160 * px, 160 * px},
    .value = lv::bind(shown),
  },
  lv::widget<"spinbox">({
    .align       = "center",
    .digit_count = 3,
    .max_value   = highest,
    .min_value   = lowest,
    .step        = 1,
    .value       = lv::bind(shown),
    .width       = 80 * px,
    .y           = -20 * px,
  }),
  lv::button(
    {
      .align                  = "center",
      .size                   = {30 * px, 30 * px},
      .x                      = -20 * px,
      .y                      = 20 * px,
      .on_clicked             = &lower,
      .on_long_pressed_repeat = &lower,
    },
    lv::label({.align = "center", .text = "-"})
  ),
  lv::button(
    {
      .align                  = "center",
      .size                   = {30 * px, 30 * px},
      .x                      = 20 * px,
      .y                      = 20 * px,
      .on_clicked             = &raise,
      .on_long_pressed_repeat = &raise,
    },
    lv::label({.align = "center", .text = "+"})
  )
);

}  // namespace

void widgets::spinbox_bind_value(lv::obj screen) { lv::place<ui>(screen); }
