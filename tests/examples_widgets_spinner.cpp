// The spinner examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_spinner_basic(void);
}

namespace {

// A quarter of a second: each arc has turned by its own speed.
void a_moment_later(lv::obj) { pass_time(250); }

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("spinner_basic", &lv_example_spinner_basic, &widgets::spinner_basic, &nothing);
  same_pixels("spinner_basic later", &lv_example_spinner_basic, &widgets::spinner_basic,
              &a_moment_later);

  // The arcs must move.
  const auto at_start = render(&widgets::spinner_basic, &nothing);
  const auto later = render(&widgets::spinner_basic, &a_moment_later);
  check(at_start.first_difference(later) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
