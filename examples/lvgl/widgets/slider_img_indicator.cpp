#include "widgets.hpp"

// Two pictures from LVGL's examples/assets: the dark segmented strip and the lit one.
extern "C" {
extern const lv_image_dsc_t img_bar_bg;  // C API
extern const lv_image_dsc_t img_bar_indicator;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

// How many of the 20 segments are lit.
constinit lv::state lit{lv::counter{20}};

// The track and the indicator are pictures, with no fill of their own and square
// corners. The knob is a small dark plate with a bright border.
constexpr auto dark_strip = lv::style({.bg_image_src = &img_bar_bg,
                                       .bg_opa = tok.opacity.none,
                                       .radius = tok.slider.image.radius});
constexpr auto lit_strip = lv::style({.bg_image_src = &img_bar_indicator,
                                      .bg_opa = tok.opacity.none,
                                      .radius = tok.slider.image.radius});
constexpr auto plate = lv::style({.bg_color = tok.slider.image.knob.fill,
                                  .bg_opa = tok.opacity.opaque,
                                  .pad_hor = tok.slider.image.knob.pad_hor,
                                  .pad_ver = tok.slider.image.knob.pad_ver,
                                  .radius = tok.slider.image.knob.radius}) |
                       tok.slider.image.knob.border;

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::slider({.max_value = 20, .min_value = 0, .size = {280 * px, 30 * px},
                .style = dark_strip, .style_indicator = lit_strip, .style_knob = plate,
                .value = lv::bind(lit)}),
    lv::label({.text = lv::bind(lit, "{}")})];

}  // namespace

void widgets::slider_img_indicator(lv::obj screen) {
  lv::place<ui>(screen);
}
