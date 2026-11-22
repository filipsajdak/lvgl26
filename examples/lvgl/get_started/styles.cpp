#include "get_started.hpp"

namespace {

using namespace get_started;
using namespace lv::units;

// A base style, an override for the pressed state, and a red variant.
constexpr auto base = lv::style({.bg_color = tok.button.fill_top,
                                 .bg_grad_color = tok.button.fill_bottom,
                                 .bg_grad_dir = "ver",
                                 .bg_opa = tok.opacity.opaque,
                                 .border_opa = tok.button.border_opacity,
                                 .radius = tok.button.radius,
                                 .text_color = tok.button.text}) |
                      tok.button.border;

constexpr auto pressed = lv::style({.bg_color = tok.button.pressed_top,
                                    .bg_grad_color = tok.button.pressed_bottom});

constexpr auto danger = lv::style({.bg_color = tok.button.danger_top,
                                   .bg_grad_color = tok.button.danger_bottom});

constexpr auto ui = lv::group[
    // The base style, and another look while pressed.
    lv::button({.align = "center", .size = {120 * px, 50 * px}, .y = -35 * px,
                .style = base, .style_pressed = pressed})[
        lv::label({.align = "center", .text = "Button"})],
    // The base style, the red variant on top of it, and a round shape.
    lv::button({.align = "center", .size = {120 * px, 50 * px},
                .y = 35 * px,
                .style = base | danger | lv::style({.radius = tok.button.pill_radius})})[
        lv::label({.align = "center", .text = "Button 2"})]];

}  // namespace

void get_started::styles(lv::obj screen) {
  lv::place<ui>(screen);
}
