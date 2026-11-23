// Ports lv_example_roller_styling. Re-ordered: lv_roller_set_options and lv_roller_set_selected
// run before lv_roller_set_visible_row_count and lv_obj_set_width.

#include "widgets.hpp"

// The large font of LVGL's examples/assets: Montserrat at 32 pixels. No token names it.
extern "C" {
extern const lv_font_t font_example_large;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

// The roller: a light gray card with roomy rows of gray text.
constexpr auto card = lv::style({
  .bg_color        = tok.roller.styled.fill,
  .bg_opa          = tok.opacity.opaque,
  .border_color    = tok.roller.styled.border,
  .border_width    = tok.roller.styled.border_width,
  .radius          = tok.roller.styled.radius,
  .text_color      = tok.roller.styled.text,
  .text_line_space = tok.roller.styled.line_space,
});

// The selected row: white text in the large font on an indigo band.
constexpr auto band = lv::style({
  .bg_color   = tok.roller.styled.selected,
  .bg_opa     = tok.opacity.opaque,
  .text_color = tok.roller.styled.selected_text,
  .text_font  = lv::raw(&font_example_large),
});

// The count of visible rows is a property, so it is set before the style: the roller's height
// is worked out with LVGL's own line space, as in the C example.
constexpr auto cities = lv::roller({
  .options           = {{"Berlin", "Paris", "Tokyo", "Sydney", "New York"}, "normal"},
  .selected          = {2, false},
  .visible_row_count = 5,
  .width             = 160 * px,
  .style             = card | lv::on("selected", band),
});

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = centered | lv::style({.pad_row = tok.gap.lg}),
  },
  cities
);

}  // namespace

void widgets::roller_styling(lv::obj screen) { lv::place<ui>(screen); }
