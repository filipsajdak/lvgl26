// The line example of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C original.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_line_points(void);
}

namespace {

void empty_screen(lv::obj) {}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("line_points", &lv_example_line_points, &widgets::line_points, &nothing);

  // The line must be in the picture.
  const auto line = render(&widgets::line_points, &nothing);
  const auto empty = render(&empty_screen, &nothing);
  check(line.first_difference(empty) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
