#include "widgets.hpp"

extern "C" {
extern const lv_image_dsc_t img_example_lvgl_logo;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

// A tile with a fill and a border, so that one can see where the picture sits in it.
constexpr auto tile = lv::style({.bg_color = tok.image.tile_fill,
                                 .bg_opa = tok.opacity.opaque,
                                 .border_color = tok.image.tile_border,
                                 .border_width = tok.image.tile_border_width});

// The same picture in three tiles of one size: at the top left, in the middle, at the bottom right.
constexpr auto ui = lv::group({.flex_flow = "row",
                               .style = spread_out | lv::style({.pad_column = tok.gap.lg})})[
    lv::widget<"image">({.inner_align = "top_left", .size = {80 * px, 80 * px},
                         .src = &img_example_lvgl_logo, .style = tile}),
    lv::widget<"image">({.inner_align = "center", .size = {80 * px, 80 * px},
                         .src = &img_example_lvgl_logo, .style = tile}),
    lv::widget<"image">({.inner_align = "bottom_right", .size = {80 * px, 80 * px},
                         .src = &img_example_lvgl_logo, .style = tile})];

}  // namespace

void widgets::image_inner_align(lv::obj screen) {
  lv::place<ui>(screen);
}
