// The buttonmatrix examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_buttonmatrix_event(void);
void lv_example_buttonmatrix_map(void);
void lv_example_buttonmatrix_one_checked(void);
void lv_example_buttonmatrix_popover(void);
void lv_example_buttonmatrix_recolor(void);
void lv_example_buttonmatrix_styling(void);
}

namespace {

// Presses the key at `index`: the matrix selects it and tells its listeners.
void press_key(lv::obj root, std::uint32_t index) {
  lv_obj_t* keys = nth(root, &lv_buttonmatrix_class);
  lv_buttonmatrix_set_selected_button(keys, index);
  lv_obj_send_event(keys, LV_EVENT_VALUE_CHANGED, nullptr);
}

void press_5(lv::obj root) { press_key(root, 4); }
void press_9(lv::obj root) { press_key(root, 8); }

// Holds the third key down: the matrix draws it in a popover above the row.
void hold_e(lv::obj root) {
  lv_obj_t* keys = nth(root, &lv_buttonmatrix_class);
  lv_buttonmatrix_set_selected_button(keys, 2);
  lv_obj_add_state(keys, LV_STATE_PRESSED);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("buttonmatrix_map", &lv_example_buttonmatrix_map, &widgets::buttonmatrix_map,
              &nothing);
  same_pixels("buttonmatrix_event", &lv_example_buttonmatrix_event, &widgets::buttonmatrix_event,
              &nothing);
  same_pixels("buttonmatrix_event key 5", &lv_example_buttonmatrix_event,
              &widgets::buttonmatrix_event, &press_5);
  same_pixels("buttonmatrix_event key 9", &lv_example_buttonmatrix_event,
              &widgets::buttonmatrix_event, &press_9);

  same_pixels("buttonmatrix_popover", &lv_example_buttonmatrix_popover,
              &widgets::buttonmatrix_popover, &nothing);
  same_pixels("buttonmatrix_popover held", &lv_example_buttonmatrix_popover,
              &widgets::buttonmatrix_popover, &hold_e);
  same_pixels("buttonmatrix_recolor", &lv_example_buttonmatrix_recolor,
              &widgets::buttonmatrix_recolor, &nothing);
  same_pixels("buttonmatrix_one_checked", &lv_example_buttonmatrix_one_checked,
              &widgets::buttonmatrix_one_checked, &nothing);
  same_pixels("buttonmatrix_styling", &lv_example_buttonmatrix_styling,
              &widgets::buttonmatrix_styling, &nothing);

  // A held key shows its popover.
  check(render(&widgets::buttonmatrix_popover, &nothing).first_difference(
            render(&widgets::buttonmatrix_popover, &hold_e)) != -1);

  // Each key must give another picture.
  const auto at_rest = render(&widgets::buttonmatrix_event, &nothing);
  const auto five = render(&widgets::buttonmatrix_event, &press_5);
  const auto nine = render(&widgets::buttonmatrix_event, &press_9);
  check(at_rest.first_difference(five) != -1);
  check(five.first_difference(nine) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
