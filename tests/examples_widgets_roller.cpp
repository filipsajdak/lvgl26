// The roller examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_roller_bind_value(void);
void lv_example_roller_event(void);
void lv_example_roller_options(void);
void lv_example_roller_visible_rows(void);
}

namespace {

// Chooses an option the way a swipe does: it selects, and tells its listeners.
void choose(lv::obj root, std::uint32_t index) {
  lv_obj_t* roller = nth(root, &lv_roller_class);
  lv_roller_set_selected(roller, index, LV_ANIM_OFF);
  lv_obj_send_event(roller, LV_EVENT_VALUE_CHANGED, nullptr);
}

void choose_fourth(lv::obj root) { choose(root, 3); }
void choose_fifth(lv::obj root) { choose(root, 4); }

void click_button(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class), LV_EVENT_CLICKED, nullptr);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("roller_options", &lv_example_roller_options, &widgets::roller_options, &nothing);
  same_pixels("roller_event", &lv_example_roller_event, &widgets::roller_event, &nothing);
  same_pixels("roller_event chosen", &lv_example_roller_event, &widgets::roller_event, &choose_fourth);
  same_pixels("roller_visible_rows", &lv_example_roller_visible_rows, &widgets::roller_visible_rows,
              &nothing);
  same_pixels("roller_bind_value", &lv_example_roller_bind_value, &widgets::roller_bind_value,
              &nothing);
  same_pixels("roller_bind_value chosen", &lv_example_roller_bind_value,
              &widgets::roller_bind_value, &choose_fifth);
  // The state keeps its value when the screen is built again, and the button moves it to Wednesday.
  same_pixels("roller_bind_value rebuilt", &lv_example_roller_bind_value,
              &widgets::roller_bind_value, &nothing);
  same_pixels("roller_bind_value button", &lv_example_roller_bind_value,
              &widgets::roller_bind_value, &click_button);

  // The label must follow the choice. The state keeps the last choice from one screen to the
  // next, so two different choices are compared.
  const auto thursday = render(&widgets::roller_event, &choose_fourth);
  const auto friday = render(&widgets::roller_event, &choose_fifth);
  check(thursday.first_difference(friday) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
