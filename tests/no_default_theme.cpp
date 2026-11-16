// Built without LVGL's default theme (LVGL26_NO_DEFAULT_THEME): a display has no theme
// until it is given one, and a theme of ours is then the only look there is.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstddef>

namespace {

using namespace lv::units;

constexpr lv_color_t lime{.blue = 58, .green = 255, .red = 212};

constexpr auto mine = lv::theme({.button = {.style = {.bg_color = lv::raw(lime), .radius = lv::raw(0 * px)}}});

constinit lv::frame<64, 48> frame;

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

void a_display_has_no_theme() {
  lv::application app;
  lv::display plain{app, frame};
  check(lv_display_get_theme(plain.raw()) == nullptr);
  lv_obj_t* button = lv_button_create(plain.screen().raw());
  check(lv_obj_get_style_radius(button, LV_PART_MAIN) == 0);  // LVGL's own button is rounded
}

void our_theme_is_the_only_look() {
  lv::application app;
  lv::display display{app, frame, mine};
  check(lv_display_get_theme(display.raw()) != nullptr);
  lv_obj_t* button = lv_button_create(display.screen().raw());
  check(same(lv_obj_get_style_bg_color(button, LV_PART_MAIN), lime));
  check(lv_obj_get_style_radius(button, LV_PART_MAIN) == 0);
}

}  // namespace

int main() {
  a_display_has_no_theme();
  our_theme_is_the_only_look();
  return failures == 0 ? 0 : 1;
}
