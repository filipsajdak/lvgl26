// The keyboard examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_keyboard_modes(void);
void lv_example_keyboard_popovers(void);
void lv_example_keyboard_textarea(void);
}

namespace {

// Holds the key at `index` down, and lets the popover appear.
void hold_key(lv::obj root, std::uint32_t index) {
  lv_obj_t* keyboard = nth(root, &lv_keyboard_class);
  lv_buttonmatrix_set_selected_button(keyboard, index);
  lv_obj_add_state(keyboard, LV_STATE_PRESSED);
  pass_time(300);
}

void hold_first_key(lv::obj root) { hold_key(root, 0); }

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("keyboard_modes", &lv_example_keyboard_modes, &widgets::keyboard_modes, &nothing);
  same_pixels("keyboard_textarea", &lv_example_keyboard_textarea, &widgets::keyboard_textarea,
              &nothing);
  same_pixels("keyboard_popovers", &lv_example_keyboard_popovers, &widgets::keyboard_popovers,
              &nothing);
  same_pixels("keyboard_popovers held", &lv_example_keyboard_popovers,
              &widgets::keyboard_popovers, &hold_first_key);

  // The number keys are another picture than the letters, and a held key shows a popover.
  const auto numbers = render(&widgets::keyboard_modes, &nothing);
  const auto letters = render(&widgets::keyboard_textarea, &nothing);
  check(numbers.first_difference(letters) != -1);
  const auto at_rest = render(&widgets::keyboard_popovers, &nothing);
  const auto held = render(&widgets::keyboard_popovers, &hold_first_key);
  check(at_rest.first_difference(held) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
