#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The look at rest: a blue fill and a soft shadow below.
constexpr auto action = lv::style({
                          .bg_color   = tok.button.action.fill,
                          .bg_opa     = tok.opacity.opaque,
                          .pad_all    = tok.button.action.pad,
                          .radius     = tok.button.action.radius,
                          .shadow_opa = opacity_step(120),
                          .text_color = tok.button.action.text,
                        }) |
                        tok.button.action.shadow;

// While pressed: darker, closer to the surface, with a ring around it.
constexpr auto pressed = lv::style({
  .bg_color        = tok.button.pressed.fill,
  .outline_color   = tok.button.pressed.ring.color,
  .outline_opa     = tok.opacity.opaque,
  .outline_pad     = tok.button.pressed.ring.pad,
  .outline_width   = tok.button.pressed.ring.width,
  .shadow_offset_y = tok.button.pressed.shadow_offset,
  .shadow_width    = tok.button.pressed.shadow_blur,
});

// While checked: green, with a wider shadow.
constexpr auto toggled = lv::style({
  .bg_color     = tok.button.toggled.fill,
  .shadow_color = tok.button.toggled.shadow_color,
  .shadow_opa   = opacity_step(160),
  .shadow_width = tok.button.toggled.shadow_blur,
});

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.xl})},
  // The look at rest, and another while pressed.
  lv::button(
    {
      .size          = {180 * px, 48 * px},
      .style         = action,
      .style_pressed = pressed,
    },
    lv::label({.align = "center", .text = "Press me"})
  ),
  // A button that stays checked: it changes to the checked look.
  lv::button(
    {
      .checkable     = true,
      .size          = {180 * px, 48 * px},
      .style         = action,
      .style_checked = toggled,
      .style_pressed = pressed,
    },
    lv::label({.align = "center", .text = "Toggle me"})
  ),
  // A style written in place, for one widget only.
  lv::button(
    {.size  = {180 * px, 48 * px},
     .style = lv::style({
                .bg_color   = tok.button.warning.fill,
                .bg_opa     = tok.opacity.opaque,
                .pad_all    = tok.button.warning.pad,
                .radius     = tok.button.warning.radius,
                .text_color = tok.button.warning.text,
              }) |
              tok.button.warning.border},
    lv::label({.align = "center", .text = "Local props"})
  )
);

}  // namespace

void widgets::button_styling(lv::obj screen) { lv::place<ui>(screen); }
