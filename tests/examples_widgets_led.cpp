// The LED examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_led_brightness(void);
void lv_example_led_color(void);
void lv_example_led_styling(void);
}

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("led_brightness", &lv_example_led_brightness, &widgets::led_brightness, &nothing);
  same_pixels("led_color", &lv_example_led_color, &widgets::led_color, &nothing);
  same_pixels("led_styling", &lv_example_led_styling, &widgets::led_styling, &nothing);

  // A different picture must be noticed.
  const auto colors = render(&widgets::led_color, &nothing);
  const auto levels = render(&widgets::led_brightness, &nothing);
  check(colors.first_difference(levels) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
