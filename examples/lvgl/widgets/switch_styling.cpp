// Ports lv_example_switch_styling. Re-ordered: on the second switch, lv_obj_set_state runs
// before lv_obj_set_size.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The track: a pale green pill with room around the knob.
constexpr auto track = lv::style({
  .bg_color     = tok.toggle.styled.track,
  .bg_opa       = tok.opacity.opaque,
  .border_width = tok.toggle.styled.border_width,
  .pad_all      = tok.toggle.styled.pad,
  .radius       = tok.toggle.styled.radius,
});

// The part of the track that shows the switch is on.
constexpr auto on = lv::style({
  .bg_color = tok.toggle.styled.on,
  .bg_opa   = tok.opacity.opaque,
});

// A white knob with a thin border and a soft shadow below it.
constexpr auto knob = lv::style({
  .bg_color        = tok.toggle.styled.knob.fill,
  .bg_opa          = tok.opacity.opaque,
  .border_color    = tok.toggle.styled.knob.border,
  .border_width    = tok.toggle.styled.knob.border_width,
  .pad_all         = tok.toggle.styled.knob.pad,
  .shadow_color    = tok.toggle.styled.knob.shadow,
  .shadow_offset_y = tok.toggle.styled.knob.shadow_offset,
  .shadow_opa      = tok.toggle.styled.knob.shadow_opacity,
  .shadow_width    = tok.toggle.styled.knob.shadow_width,
});

constexpr auto look = track | lv::on("indicator", "checked", on) | lv::on("knob", knob);

// The size of each switch in the C example.
constexpr auto width = 60 * px;
constexpr auto height = 30 * px;

// Two switches with the same look; the second is on.
constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.gap.lg}),
  },
  lv::toggle({.size = {width, height}, .style = look}),
  lv::toggle({.checked = true, .size = {width, height}, .style = look})
);

}  // namespace

void widgets::switch_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
