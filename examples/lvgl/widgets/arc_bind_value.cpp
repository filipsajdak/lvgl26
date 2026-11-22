#include "widgets.hpp"

#include <mp-units/systems/si.h>

namespace {

using namespace widgets;
using namespace lv::units;

// One value for the arc, the label inside it and the slider: move the slider and
// the other two follow.
// The C example also limits its subject to 0..100. A state has no limits; the
// slider's own range keeps the value inside them here.
constinit lv::state level{50 * mp_units::percent};

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    // The arc only shows the value: it takes no clicks and has no knob.
    lv::arc({.clickable = false, .size = {160 * px, 160 * px},
             .value = lv::bind(level), .style_knob = {.bg_opa = tok.opacity.none}})[
        lv::label({.align = "center", .text = lv::bind(level)})],  // "50 %"
    lv::slider({.value = lv::bind(level), .width = 220 * px})];

}  // namespace

void widgets::arc_bind_value(lv::obj screen) {
  lv::place<ui>(screen);
}
