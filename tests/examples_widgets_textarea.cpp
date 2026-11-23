// The textarea examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_textarea_cursor(void);
void lv_example_textarea_placeholder(void);
}

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("textarea_placeholder", &lv_example_textarea_placeholder,
              &widgets::textarea_placeholder, &nothing);
  same_pixels("textarea_cursor", &lv_example_textarea_cursor, &widgets::textarea_cursor, &nothing);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
