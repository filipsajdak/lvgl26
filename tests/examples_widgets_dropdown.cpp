// The dropdown examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_dropdown_bind_value(void);
void lv_example_dropdown_direction(void);
void lv_example_dropdown_event(void);
void lv_example_dropdown_text(void);
}

namespace {

// Chooses an option the way a click on the list does: it selects, and tells its listeners.
void choose(lv::obj root, std::uint32_t index) {
  lv_obj_t* list = nth(root, &lv_dropdown_class);
  lv_dropdown_set_selected(list, index);
  lv_obj_send_event(list, LV_EVENT_VALUE_CHANGED, nullptr);
}

void choose_third(lv::obj root) { choose(root, 2); }
void choose_last(lv::obj root) { choose(root, 3); }

void click_button(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class), LV_EVENT_CLICKED, nullptr);
}

// Opens the first dropdown, and waits for the list to finish its animation.
void open_first(lv::obj root) {
  lv_dropdown_open(nth(root, &lv_dropdown_class));
  pass_time(500);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("dropdown_direction", &lv_example_dropdown_direction, &widgets::dropdown_direction,
              &nothing);
  same_pixels("dropdown_direction open", &lv_example_dropdown_direction,
              &widgets::dropdown_direction, &open_first);
  same_pixels("dropdown_text", &lv_example_dropdown_text, &widgets::dropdown_text, &nothing);
  same_pixels("dropdown_text third", &lv_example_dropdown_text, &widgets::dropdown_text,
              &choose_third);
  same_pixels("dropdown_event", &lv_example_dropdown_event, &widgets::dropdown_event, &nothing);
  same_pixels("dropdown_event third", &lv_example_dropdown_event, &widgets::dropdown_event,
              &choose_third);
  same_pixels("dropdown_event last", &lv_example_dropdown_event, &widgets::dropdown_event,
              &choose_last);
  same_pixels("dropdown_event rebuilt", &lv_example_dropdown_event, &widgets::dropdown_event,
              &nothing);
  same_pixels("dropdown_bind_value", &lv_example_dropdown_bind_value,
              &widgets::dropdown_bind_value, &nothing);
  same_pixels("dropdown_bind_value chosen", &lv_example_dropdown_bind_value,
              &widgets::dropdown_bind_value, &choose_last);
  // Both keep their value when built again, and the button moves it to Cherry.
  same_pixels("dropdown_bind_value rebuilt", &lv_example_dropdown_bind_value,
              &widgets::dropdown_bind_value, &nothing);
  same_pixels("dropdown_bind_value button", &lv_example_dropdown_bind_value,
              &widgets::dropdown_bind_value, &click_button);

  // A chosen option must be a different picture from the first.
  const auto first = render(&widgets::dropdown_event, &nothing);
  const auto third = render(&widgets::dropdown_event, &choose_third);
  check(first.first_difference(third) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
