// The buttonmatrix examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_buttonmatrix_event(void);
void lv_example_buttonmatrix_map(void);
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
