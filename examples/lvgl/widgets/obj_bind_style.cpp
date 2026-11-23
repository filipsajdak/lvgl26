#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Whether the panel has its dark look. The button flips it.
constinit lv::state<bool> dark{false};

void flip_theme() {
  dark.update([](bool now) {
    return !now;
  });
}

// One difference from the C example: that one adds a style to the panel while the
// flag is set (lv_obj_bind_style). A style cannot follow a state here. Instead the
// panel's checked state follows the flag, and the dark look is its style for that
// state. The picture is the same; the panel is also "checked" for anyone who asks.
constexpr auto dark_look = lv::style({
  .bg_color     = tok.panel.dark.fill,
  .border_color = tok.panel.dark.border,
  .text_color   = tok.panel.dark.text,
});

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::box(
    {
      .checked = lv::bind(dark),
      .size    = {240 * px, 120 * px},
      .style = {
        .border_width = tok.panel.border_width,
        .pad_all      = tok.panel.pad,
        .radius       = tok.panel.radius,
      },
      .style_checked = dark_look,
    },
    lv::label({.align = "center", .text = "Panel content"})
  ),
  lv::button({.on_clicked = &flip_theme}, lv::label({.align = "center", .text = "Toggle theme"}))
);

}  // namespace

void widgets::obj_bind_style(lv::obj screen) { lv::place<ui>(screen); }
