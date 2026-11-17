// Each get_started example, rewritten with lvgl26, must render the same pixels
// as LVGL's C original.
//
// This test also guards a linker trap: the four ports each have a tree called `ui`
// in an unnamed namespace. If the code generated for one were shared with another,
// the pictures would show the wrong widgets.

#include "../examples/lvgl/get_started/get_started.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_get_started_hello_world(void);
void lv_example_get_started_slider(void);
void lv_example_get_started_styles(void);
void lv_example_get_started_button(void);
}

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("hello_world", &lv_example_get_started_hello_world, &get_started::hello_world, &nothing);
  same_pixels("slider", &lv_example_get_started_slider, &get_started::slider, &nothing);
  same_pixels("styles", &lv_example_get_started_styles, &get_started::styles, &nothing);
  // The two differ before the first click (see button.cpp), so compare after one.
  same_pixels("button", &lv_example_get_started_button, &get_started::button, &click_first_button);

  // A different picture must be noticed.
  const auto hello = render(&get_started::hello_world, &nothing);
  const auto styled = render(&get_started::styles, &nothing);
  check(hello.first_difference(styled) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
