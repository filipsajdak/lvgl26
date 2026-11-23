#include "widgets.hpp"

// Two pictures from LVGL's examples/assets: the dark segmented strip and the lit one.
extern "C" {
extern const lv_image_dsc_t img_bar_bg;         // C API
extern const lv_image_dsc_t img_bar_indicator;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

// How many segments are lit. The bar shows up to 20.
constinit lv::state lit{lv::counter{20}};

// The count stays between 0 and 100, the limits the C example gives its subject.
void one_less() {
  lit.update([](lv::counter c) {
    return c.value > 0 ? --c : c;
  });
}

void one_more() {
  lit.update([](lv::counter c) {
    return c.value < 100 ? ++c : c;
  });
}

// Each part is a picture, with no fill of its own and square corners.
constexpr auto dark_strip = lv::style({
  .bg_image_src = &img_bar_bg,
  .bg_opa       = tok.opacity.none,
  .radius       = tok.bar.image.radius,
});
constexpr auto lit_strip  = lv::style({
   .bg_image_src = &img_bar_indicator,
   .bg_opa       = tok.opacity.none,
   .radius       = tok.bar.image.radius,
});

// The row of the two buttons: a box without its own look.
constexpr auto stepper = lv::style({
  .bg_opa           = tok.opacity.none,
  .border_width     = tok.bar.stepper.border_width,
  .flex_cross_place = "center",
  .flex_main_place  = "space_between",
  .pad_column       = tok.bar.stepper.gap,
});

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::bar({
    .max_value       = 20,
    .min_value       = 0,
    .size            = {280 * px, 35 * px},
    .value           = lv::bind(lit),
    .style           = dark_strip,
    .style_indicator = lit_strip,
  }),
  lv::row(
    {.size = {180 * px, lv::size_content}, .style = stepper},
    lv::button(
      {.on_clicked = &one_less, .on_long_pressed_repeat = &one_less},
      lv::label({.align = "center", .text = "-"})
    ),
    lv::label({.text = lv::bind(lit, "{}")}),
    lv::button(
      {.on_clicked = &one_more, .on_long_pressed_repeat = &one_more},
      lv::label({.align = "center", .text = "+"})
    )
  )
);

}  // namespace

void widgets::bar_img_indicator(lv::obj screen) { lv::place<ui>(screen); }
