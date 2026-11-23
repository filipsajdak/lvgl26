// Ports lv_example_textarea_cursor. Re-ordered: lv_obj_set_state (focused) runs first, and
// lv_obj_set_width after lv_textarea_set_text.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A block cursor: the character under it in white on blue.
constexpr auto block = lv::style({
  .bg_color    = tok.textarea.cursor.block.fill,
  .bg_opa      = tok.opacity.opaque,
  .border_side = "none",
  .text_color  = tok.textarea.cursor.block.text,
});

// A bar cursor: a red line at the left of the character.
constexpr auto bar = lv::style({
  .bg_opa       = tok.opacity.none,
  .border_color = tok.textarea.cursor.bar.color,
  .border_side  = "left",
  .border_width = tok.textarea.cursor.bar.width,
});

// Two focused one-line fields with the cursor after their second character.
constexpr auto block_field = lv::textarea({
  .cursor_pos = 2,
  .focused    = true,
  .one_line   = true,
  .text       = "Block",
  .width      = 60 * pct,
  .style      = lv::on("cursor", "focused", block),
});

constexpr auto bar_field = lv::textarea({
  .cursor_pos = 2,
  .focused    = true,
  .one_line   = true,
  .text       = "Bar",
  .width      = 60 * pct,
  .style      = lv::on("cursor", "focused", bar),
});

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.gap.lg}),
  },
  block_field,
  bar_field
);

}  // namespace

void widgets::textarea_cursor(lv::obj screen) {
  lv::place<ui>(screen);
}
