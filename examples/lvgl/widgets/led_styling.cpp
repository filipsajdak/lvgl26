#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A round LED with a ring close around it and a soft glow.
constexpr auto ok = lv::widget<"led">(
    {.color = tok.led.ok.color, .size = {50 * px, 50 * px},
     .style = lv::style({.outline_color = tok.led.ok.color,
                         .outline_opa = opacity_step(80),
                         .outline_pad = tok.led.ok.ring.pad,
                         .outline_width = tok.led.ok.ring.width,
                         .shadow_color = tok.led.ok.color,
                         .shadow_opa = opacity_step(200),
                         .shadow_width = tok.led.ok.glow}) |
              tok.led.ok.border});

// A nearly square LED with a hard border and no glow.
constexpr auto fault = lv::widget<"led">(
    {.color = tok.led.fault.color, .size = {50 * px, 50 * px},
     .style = lv::style({.radius = tok.led.fault.radius, .shadow_width = tok.led.no_glow}) |
              tok.led.fault.border});

// A dimmed LED with a ring further out: something is pending.
constexpr auto pending = lv::widget<"led">(
    {.brightness = 120, .color = tok.led.pending.color, .size = {50 * px, 50 * px},
     .style = {.outline_color = tok.led.pending.color,
               .outline_opa = opacity_step(180),
               .outline_pad = tok.led.pending.ring.pad,
               .outline_width = tok.led.pending.ring.width,
               .shadow_width = tok.led.no_glow}});

}  // namespace

// The screen itself is the row, as in the C example: the gap between the LEDs is
// the one the theme gives a screen. A tree has one root, so each LED is its own.
void widgets::led_styling(lv::obj screen) {
  screen.set({.flex_flow = "row",
              .style = spread_out | lv::style({.flex_cross_place = "center"})});
  lv::place<ok>(screen);
  lv::place<fault>(screen);
  lv::place<pending>(screen);
}
