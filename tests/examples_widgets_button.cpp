// The button examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_button_basic(void);
void lv_example_button_event(void);
void lv_example_button_states(void);
void lv_example_button_styling(void);
}

namespace {

// Holds the first button down, and waits until the theme's transition is over.
void press_first(lv::obj root) {
  lv_obj_add_state(nth(root, &lv_button_class, 0), LV_STATE_PRESSED);
  pass_time(500);
}

// Checks the second button and holds it down as well.
void check_and_press_second(lv::obj root) {
  lv_obj_t* button = nth(root, &lv_button_class, 1);
  lv_obj_add_state(button, LV_STATE_CHECKED);
  lv_obj_add_state(button, LV_STATE_PRESSED);
  pass_time(500);
}

void check_second(lv::obj root) {
  lv_obj_add_state(nth(root, &lv_button_class, 1), LV_STATE_CHECKED);
  pass_time(500);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("button_basic", &lv_example_button_basic, &widgets::button_basic, &nothing);
  // Before the first click, then after one and after two.
  same_pixels("button_event", &lv_example_button_event, &widgets::button_event, &nothing);
  same_pixels("button_event 1 click", &lv_example_button_event, &widgets::button_event,
              &click_first_button);
  same_pixels("button_event 2 clicks", &lv_example_button_event, &widgets::button_event,
              &click_first_button);
  same_pixels("button_states", &lv_example_button_states, &widgets::button_states, &nothing);
  same_pixels("button_styling", &lv_example_button_styling, &widgets::button_styling, &nothing);
  same_pixels("button_styling pressed", &lv_example_button_styling, &widgets::button_styling,
              &press_first);
  same_pixels("button_styling checked", &lv_example_button_styling, &widgets::button_styling,
              &check_second);
  same_pixels("button_styling both", &lv_example_button_styling, &widgets::button_styling,
              &check_and_press_second);

  // The pressed look must be a different picture from the look at rest.
  const auto at_rest = render(&widgets::button_styling, &nothing);
  const auto held = render(&widgets::button_styling, &press_first);
  const auto on = render(&widgets::button_styling, &check_second);
  check(at_rest.first_difference(held) != -1);
  check(at_rest.first_difference(on) != -1);
  check(held.first_difference(on) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
