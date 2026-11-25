// Ports lv_example_buttonmatrix_styling. Re-ordered: lv_buttonmatrix_set_map runs first, and
// lv_buttonmatrix_set_one_checked before lv_obj_set_size. The controls, set by
// lv_buttonmatrix_set_button_ctrl and lv_buttonmatrix_set_button_ctrl_all in place of
// lv_buttonmatrix_set_ctrl_map, run after the styles.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The panel: white, rounded, with a thin border and room around the keys.
constexpr auto panel = lv::style({
  .bg_color     = tok.buttonmatrix.styled.panel.fill,
  .bg_opa       = tok.opacity.opaque,
  .border_color = tok.buttonmatrix.styled.panel.border,
  .border_width = tok.buttonmatrix.styled.panel.border_width,
  .pad_all      = tok.buttonmatrix.styled.panel.pad,
  .pad_column   = tok.buttonmatrix.styled.panel.pad,
  .pad_row      = tok.buttonmatrix.styled.panel.pad,
  .radius       = tok.buttonmatrix.styled.panel.radius,
});

// A key: light gray with a gray border.
constexpr auto key = lv::style({
  .bg_color     = tok.buttonmatrix.styled.key.fill,
  .bg_opa       = tok.opacity.opaque,
  .border_color = tok.buttonmatrix.styled.key.border,
  .border_width = tok.buttonmatrix.styled.key.border_width,
  .radius       = tok.buttonmatrix.styled.key.radius,
  .text_color   = tok.buttonmatrix.styled.key.text,
});

// The checked key: indigo, with white text and a faint ring around it.
constexpr auto checked_key = lv::style({
  .bg_color      = tok.buttonmatrix.styled.checked.fill,
  .outline_color = tok.buttonmatrix.styled.checked.ring,
  .outline_opa   = opacity_step(80),
  .outline_pad   = tok.buttonmatrix.styled.checked.ring_pad,
  .outline_width = tok.buttonmatrix.styled.checked.ring_width,
  .text_color    = tok.buttonmatrix.styled.checked.text,
});

// The days of a week. Every key can be checked, one at a time, and Wednesday is.
constexpr const char* const days[] = {"M", "T", "W", "T", "F", "S", "S", nullptr};

constexpr auto week = lv::widget<"buttonmatrix">({
  .button_ctrl     = {2, "checked"},
  .button_ctrl_all = "checkable",
  .map             = days,
  .one_checked     = true,
  .size            = {90 * pct, 80 * px},
  .style           = panel | lv::on("items", key) | lv::on("items", "checked", checked_key),
});

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.gap.lg}),
  },
  week
);

}  // namespace

void widgets::buttonmatrix_styling(lv::obj screen) { lv::place<ui>(screen); }
