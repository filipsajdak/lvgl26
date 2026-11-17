// The scale examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_scale_modes(void);
void lv_example_scale_range(void);
void lv_example_scale_styling(void);
void lv_example_scale_ticks(void);
}

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("scale_modes", &lv_example_scale_modes, &widgets::scale_modes, &nothing);
  same_pixels("scale_range", &lv_example_scale_range, &widgets::scale_range, &nothing);
  same_pixels("scale_styling", &lv_example_scale_styling, &widgets::scale_styling, &nothing);
  same_pixels("scale_ticks", &lv_example_scale_ticks, &widgets::scale_ticks, &nothing);

  // Another range gives other labels.
  const auto range = render(&widgets::scale_range, &nothing);
  const auto ticks = render(&widgets::scale_ticks, &nothing);
  check(range.first_difference(ticks) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
