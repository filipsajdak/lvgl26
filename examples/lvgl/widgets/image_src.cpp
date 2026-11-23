#include "widgets.hpp"

extern "C" {
extern const lv_image_dsc_t img_example_lvgl_logo;  // C API
}

namespace {

using namespace widgets;

// LVGL's logo, from the C array that the original uses, in the middle of the screen.
constexpr auto ui = lv::widget<"image">({.align = "center", .src = &img_example_lvgl_logo});

}  // namespace

void widgets::image_src(lv::obj screen) { lv::place<ui>(screen); }
