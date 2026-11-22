// Ports lv_example_checkbox_styling. Re-ordered: lv_checkbox_set_text runs last, after the
// styles and, on the second checkbox, after lv_obj_set_state.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The box of a checkbox: a light square with a gray border.
constexpr auto box = lv::style({
  .bg_color     = tok.checkbox.styled.box.fill,
  .bg_opa       = tok.opacity.opaque,
  .border_color = tok.checkbox.styled.box.border,
  .border_width = tok.checkbox.styled.box.border_width,
  .pad_all      = tok.checkbox.styled.box.pad,
  .radius       = tok.checkbox.styled.box.radius,
});

// The box while ticked: green, with a faint ring around it.
constexpr auto ticked = lv::style({
  .bg_color      = tok.checkbox.styled.ticked.fill,
  .border_color  = tok.checkbox.styled.ticked.border,
  .outline_color = tok.checkbox.styled.ticked.ring,
  .outline_opa   = opacity_step(80),
  .outline_pad   = tok.checkbox.styled.ticked.ring_pad,
  .outline_width = tok.checkbox.styled.ticked.ring_width,
});

// Both checkboxes look the same; the second starts ticked.
constexpr auto look = lv::style({
                        .pad_column = tok.checkbox.styled.gap,
                        .text_color = tok.checkbox.styled.text,
                      }) |
                      lv::on("indicator", box) | lv::on("indicator", "checked", ticked);

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.checkbox.styled.row_gap}),
  },
  lv::checkbox({.style = look, .text = "Default look"}),
  lv::checkbox({.checked = true, .style = look, .text = "Pre-checked"})
);

}  // namespace

void widgets::checkbox_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
