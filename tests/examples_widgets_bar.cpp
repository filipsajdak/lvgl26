// The bar examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_bar_bind_value(void);
void lv_example_bar_img_indicator(void);
void lv_example_bar_modes(void);
void lv_example_bar_orientation(void);
void lv_example_bar_styling(void);
void lv_example_bar_value_range(void);
}

namespace {

// The "-" button is the first one, the "+" button the second.
void click(lv::obj root, int button, int times) {
  for (int i = 0; i < times; ++i)
    lv_obj_send_event(nth(root, &lv_button_class, button), LV_EVENT_CLICKED, nullptr);
}

void plus_once(lv::obj root) { click(root, 1, 1); }
// From 55 down past 0: the value starts again from 100.
void minus_past_the_start(lv::obj root) { click(root, 0, 12); }
// From 100 up past 100: the value starts again from 0, then goes on to 5.
void plus_past_the_end(lv::obj root) { click(root, 1, 2); }
// Holding a button down repeats the step.
void hold_plus(lv::obj root) {
  for (int i = 0; i < 3; ++i)
    lv_obj_send_event(nth(root, &lv_button_class, 1), LV_EVENT_LONG_PRESSED_REPEAT, nullptr);
}
// From 20 down: the count stops at 0.
void minus_to_the_stop(lv::obj root) { click(root, 0, 25); }
// From 0 up: the count goes on past the 20 segments, and stops at 100.
void plus_to_the_stop(lv::obj root) { click(root, 1, 105); }
void minus_once(lv::obj root) { click(root, 0, 1); }

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("bar_bind_value", &lv_example_bar_bind_value, &widgets::bar_bind_value, &nothing);
  same_pixels("bar_bind_value +", &lv_example_bar_bind_value, &widgets::bar_bind_value,
              &plus_once);
  same_pixels("bar_bind_value below 0", &lv_example_bar_bind_value, &widgets::bar_bind_value,
              &minus_past_the_start);
  same_pixels("bar_bind_value above 100", &lv_example_bar_bind_value, &widgets::bar_bind_value,
              &plus_past_the_end);
  same_pixels("bar_bind_value held", &lv_example_bar_bind_value, &widgets::bar_bind_value,
              &hold_plus);
  same_pixels("bar_img_indicator", &lv_example_bar_img_indicator, &widgets::bar_img_indicator,
              &nothing);
  same_pixels("bar_img_indicator at 0", &lv_example_bar_img_indicator,
              &widgets::bar_img_indicator, &minus_to_the_stop);
  same_pixels("bar_img_indicator at 100", &lv_example_bar_img_indicator,
              &widgets::bar_img_indicator, &plus_to_the_stop);
  same_pixels("bar_img_indicator at 99", &lv_example_bar_img_indicator,
              &widgets::bar_img_indicator, &minus_once);
  same_pixels("bar_modes", &lv_example_bar_modes, &widgets::bar_modes, &nothing);
  same_pixels("bar_orientation", &lv_example_bar_orientation, &widgets::bar_orientation, &nothing);
  same_pixels("bar_styling", &lv_example_bar_styling, &widgets::bar_styling, &nothing);
  same_pixels("bar_value_range", &lv_example_bar_value_range, &widgets::bar_value_range, &nothing);

  // A step must be a different picture.
  const auto before = render(&widgets::bar_bind_value, &nothing);
  const auto after = render(&widgets::bar_bind_value, &plus_once);
  check(before.first_difference(after) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
