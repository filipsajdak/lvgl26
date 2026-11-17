#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The line along the scale.
constexpr auto axis = lv::style({.line_color = tok.scale.axis, .line_width = tok.scale.axis_width});

// The minor ticks.
constexpr auto minor = lv::style({.line_color = tok.scale.minor, .line_width = tok.scale.minor_width});

// The major ticks and their labels: in the accent color, and the labels tilted.
constexpr auto accent = lv::style({.line_color = tok.scale.accent,
                                   .line_width = tok.scale.accent_width,
                                   .pad_bottom = tok.scale.label_pad,
                                   .text_color = tok.scale.accent,
                                   .transform_rotation = tok.scale.label_tilt});

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::widget<"scale">({.label_show = true, .major_tick_every = 4, .max_value = 100, .min_value = 0,
               .mode = "horizontal_bottom", .size = {280 * px, 80 * px}, .style = axis,
               .style_indicator = accent, .style_items = minor, .total_tick_count = 21})];

}  // namespace

void widgets::scale_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
