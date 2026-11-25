// Ports lv_example_slider_styling. Re-ordered: lv_slider_set_max_value runs before
// lv_slider_set_min_value, lv_obj_set_size after both, and the styles before lv_slider_set_value.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The track: dark, rounded, with a thin border.
constexpr auto track = lv::style({
  .bg_color     = tok.slider.styled.track.fill,
  .bg_opa       = tok.opacity.opaque,
  .border_color = tok.slider.styled.track.border,
  .border_width = tok.slider.styled.track.border_width,
  .radius       = tok.slider.styled.track.radius,
});

// The filled part: a gradient from violet to pink, with a glow under it.
constexpr auto indicator = lv::style({
  .bg_color      = tok.slider.styled.indicator.from,
  .bg_grad_color = tok.slider.styled.indicator.to,
  .bg_grad_dir   = "hor",
  .bg_opa        = tok.opacity.opaque,
  .radius        = tok.slider.styled.indicator.radius,
  .shadow_color  = tok.slider.styled.indicator.shadow,
  .shadow_opa    = opacity_step(150),
  .shadow_spread = tok.slider.styled.indicator.shadow_spread,
  .shadow_width  = tok.slider.styled.indicator.shadow_width,
});

// A white round knob with a violet border and a small shadow.
constexpr auto knob = lv::style({
  .bg_color        = tok.slider.styled.knob.fill,
  .bg_opa          = tok.opacity.opaque,
  .border_color    = tok.slider.styled.knob.border,
  .border_width    = tok.slider.styled.knob.border_width,
  .pad_all         = tok.slider.styled.knob.pad,
  .radius          = tok.slider.styled.knob.radius,
  .shadow_color    = tok.slider.styled.knob.shadow,
  .shadow_offset_y = tok.slider.styled.knob.shadow_offset,
  .shadow_opa      = opacity_step(80),
  .shadow_width    = tok.slider.styled.knob.shadow_width,
});

// While the knob is held: a warmer fill and a pink ring around it.
constexpr auto held = lv::style({
  .bg_color      = tok.slider.styled.pressed.fill,
  .outline_color = tok.slider.styled.pressed.ring,
  .outline_opa   = opacity_step(120),
  .outline_pad   = tok.slider.styled.pressed.ring_pad,
  .outline_width = tok.slider.styled.pressed.ring_width,
});

constexpr auto look =
  track | lv::on("indicator", indicator) | lv::on("knob", knob) | lv::on("knob", "pressed", held);

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.gap.xl}),
  },
  lv::slider({
    .max_value = 100,
    .min_value = 0,
    .size      = {90 * pct, 22 * px},
    .value     = {55, false},
    .style     = look,
  })
);

}  // namespace

void widgets::slider_styling(lv::obj screen) { lv::place<ui>(screen); }
