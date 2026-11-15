#include "widgets.hpp"

#include <mp-units/systems/si.h>

namespace {

using namespace widgets;
using namespace lv::units;
using mp_units::percent;

// The value of the bar and of the label between the two buttons.
constinit lv::state level{50 * percent};

// One step down or up. Past either end the value starts again from the other.
// The C example gives these rules to LVGL's increment event; here they are code.
void step_down() {
  level.update([](auto v) {
    v -= 5 * percent;
    return v < 0 * percent ? 100 * percent : v;
  });
}

void step_up() {
  level.update([](auto v) {
    v += 5 * percent;
    return v > 100 * percent ? 0 * percent : v;
  });
}

// The row of the two buttons: a box without its own look.
constexpr auto stepper = lv::style({.bg_opa = tok.opacity.none,
                                    .border_width = tok.bar.stepper.border_width,
                                    .flex_cross_place = LV_FLEX_ALIGN_CENTER,
                                    .flex_main_place = LV_FLEX_ALIGN_SPACE_BETWEEN,
                                    .pad_column = tok.bar.stepper.gap});

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::bar({.size = {90 * pct, 20 * px}, .value = lv::bind(level)}),
    lv::row({.size = {180 * px, lv::size_content}, .style = stepper})[
        // A click steps once; holding the button down repeats the step.
        lv::button({.on_clicked = &step_down, .on_long_pressed_repeat = &step_down})[
            lv::label({.align = LV_ALIGN_CENTER, .text = "-"})],
        lv::label({.text = lv::bind(level, "{}%")}),
        lv::button({.on_clicked = &step_up, .on_long_pressed_repeat = &step_up})[
            lv::label({.align = LV_ALIGN_CENTER, .text = "+"})]]];

}  // namespace

void widgets::bar_bind_value(lv::obj screen) {
  lv::place<ui>(screen);
}
