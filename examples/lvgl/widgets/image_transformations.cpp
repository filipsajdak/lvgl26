#include "widgets.hpp"

extern "C" {
extern const lv_image_dsc_t img_example_lvgl_logo;  // C API
}

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto plain = lv::widget<"image">({.src = &img_example_lvgl_logo});

// Turned by 45 degrees.
constexpr auto turned = lv::widget<"image">({.rotation = 45 * deg, .src = &img_example_lvgl_logo});

// Scaled to 160 of 256 along both axes.
constexpr auto scaled = lv::widget<"image">({
  .scale_x = 160,
  .scale_y = 160,
  .src     = &img_example_lvgl_logo,
});

}  // namespace

// The screen itself is the row, as in the C example: the gap between the pictures is
// the one the theme gives a screen. A tree has one root, so each picture is its own.
void widgets::image_transformations(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<plain>(screen);
  lv::place<turned>(screen);
  lv::place<scaled>(screen);
}
