// The spinbox examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_spinbox_bind_value(void);
void lv_example_spinbox_format(void);
void lv_example_spinbox_rollover(void);
void lv_example_spinbox_styling(void);
void lv_example_spinbox_value(void);
}

namespace {

void click_plus(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class, 1), LV_EVENT_CLICKED, nullptr);
}

void click_minus(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class, 0), LV_EVENT_CLICKED, nullptr);
}

void hold_plus(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class, 1), LV_EVENT_LONG_PRESSED_REPEAT, nullptr);
}

// Holds the minus button for longer than the value can fall: it stops at 0.
void minus_to_the_end(lv::obj root) {
  for (int i = 0; i < 120; ++i) click_minus(root);
}

void plus_to_the_end(lv::obj root) {
  for (int i = 0; i < 120; ++i) click_plus(root);
}

void increment(lv::obj root) { lv_spinbox_increment(nth(root, &lv_spinbox_class)); }

void increment_twice(lv::obj root) {
  increment(root);
  increment(root);
}

void decrement(lv::obj root) { lv_spinbox_decrement(nth(root, &lv_spinbox_class)); }

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("spinbox_format", &lv_example_spinbox_format, &widgets::spinbox_format, &nothing);
  same_pixels("spinbox_styling", &lv_example_spinbox_styling, &widgets::spinbox_styling, &nothing);
  same_pixels("spinbox_value", &lv_example_spinbox_value, &widgets::spinbox_value, &nothing);
  same_pixels("spinbox_value step", &lv_example_spinbox_value, &widgets::spinbox_value,
              &increment);
  same_pixels("spinbox_rollover", &lv_example_spinbox_rollover, &widgets::spinbox_rollover,
              &nothing);
  same_pixels("spinbox_rollover wraps up", &lv_example_spinbox_rollover,
              &widgets::spinbox_rollover, &increment_twice);
  same_pixels("spinbox_rollover wraps down", &lv_example_spinbox_rollover,
              &widgets::spinbox_rollover, &decrement);
  same_pixels("spinbox_bind_value", &lv_example_spinbox_bind_value, &widgets::spinbox_bind_value,
              &nothing);
  same_pixels("spinbox_bind_value plus", &lv_example_spinbox_bind_value,
              &widgets::spinbox_bind_value, &click_plus);
  same_pixels("spinbox_bind_value hold", &lv_example_spinbox_bind_value,
              &widgets::spinbox_bind_value, &hold_plus);
  same_pixels("spinbox_bind_value minus", &lv_example_spinbox_bind_value,
              &widgets::spinbox_bind_value, &click_minus);
  same_pixels("spinbox_bind_value at 0", &lv_example_spinbox_bind_value,
              &widgets::spinbox_bind_value, &minus_to_the_end);
  same_pixels("spinbox_bind_value at 100", &lv_example_spinbox_bind_value,
              &widgets::spinbox_bind_value, &plus_to_the_end);

  const auto at_rest = render(&widgets::spinbox_rollover, &nothing);
  const auto wrapped = render(&widgets::spinbox_rollover, &increment_twice);
  check(at_rest.first_difference(wrapped) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
