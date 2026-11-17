// The base object examples of LVGL's examples/widgets, rewritten with lvgl26, must
// render the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_obj_bind_style(void);
}

namespace {

void click_once(lv::obj root) {
  click_first_button(root);
  pass_time(500);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  // Light, then dark after one click, then light again after another.
  same_pixels("obj_bind_style", &lv_example_obj_bind_style, &widgets::obj_bind_style, &nothing);
  same_pixels("obj_bind_style dark", &lv_example_obj_bind_style, &widgets::obj_bind_style,
              &click_once);
  const auto dark = render(&widgets::obj_bind_style, &nothing);
  same_pixels("obj_bind_style light", &lv_example_obj_bind_style, &widgets::obj_bind_style,
              &click_once);
  const auto light = render(&widgets::obj_bind_style, &nothing);
  check(dark.first_difference(light) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
