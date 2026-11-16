#include "widgets.hpp"

// Two pictures from LVGL's examples/assets: the dark segmented ring and the lit one.
extern "C" {
extern const lv_image_dsc_t img_arc_bg;  // C API
extern const lv_image_dsc_t img_arc_indicator;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

// How many of the 22 segments are lit.
constinit lv::state lit{lv::counter{20}};

// Both parts are drawn with a picture instead of a color: the indicator shows as
// much of the lit ring as the value says. The knob is invisible.
constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.none})})[
    lv::arc({.bg_end_angle = 41 * deg, .bg_start_angle = 138 * deg, .max_value = 22,
             .min_value = 0, .size = {200 * px, 200 * px},
             .style = {.arc_image_src = &img_arc_bg, .arc_rounded = false,
                       .arc_width = tok.arc.image.width},
             .style_indicator = {.arc_image_src = &img_arc_indicator, .arc_rounded = false,
                                 .arc_width = tok.arc.image.width},
             .style_knob = {.opa = tok.opacity.none},
             .value = lv::bind(lit)})[
        lv::label({.align = "center", .text = lv::bind(lit, "{}")})]];

}  // namespace

void widgets::arc_img_indicator(lv::obj screen) {
  lv::place<ui>(screen);
}
