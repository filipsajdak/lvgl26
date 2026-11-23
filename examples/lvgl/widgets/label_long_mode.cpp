#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// What every sample has: a filled, rounded background. Each adds its own color.
constexpr auto sample =
  lv::style({.bg_opa = tok.opacity.opaque, .radius = tok.label.sample.radius});

// One label per way of handling a text that is too long for it.
constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.md})},
  lv::label({
    .long_mode = "wrap",
    .text      = "Wrap keeps all text visible by splitting long lines into multiple rows.",
    .width     = 170 * px,
    .style     = sample | lv::style({.bg_color = tok.label.sample.wrap}),
  }),
  lv::label({
    .long_mode = "dots",
    .size      = {170 * px, 32 * px},
    .text      = "Dots shortens overflowing text and replaces the tail with '...'",
    .style     = sample | lv::style({.bg_color = tok.label.sample.dots}),
  }),
  lv::label({
    .long_mode = "scroll",
    .text      = "Scroll moves overflowing text back and forth inside the label area.",
    .width     = 170 * px,
    .style     = sample | lv::style({.bg_color = tok.label.sample.scroll}),
  }),
  lv::label({
    .long_mode = "scroll_circular",
    .text      = "Scroll circular continuously loops overflowing text forever.",
    .width     = 170 * px,
    .style     = sample | lv::style({.bg_color = tok.label.sample.circular}),
  }),
  lv::label({
    .long_mode = "clip",
    .text      = "Clip simply cuts any part that falls outside the label width.",
    .width     = 170 * px,
    .style     = sample | lv::style({.bg_color = tok.label.sample.clip}),
  })
);

}  // namespace

void widgets::label_long_mode(lv::obj screen) { lv::place<ui>(screen); }
