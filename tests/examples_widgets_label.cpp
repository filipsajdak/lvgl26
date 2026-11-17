// The label examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_label_bind_text(void);
void lv_example_label_long_mode(void);
void lv_example_label_recolor(void);
void lv_example_label_set_text_newline(void);
void lv_example_label_styling(void);
void lv_example_label_text_align(void);
}

namespace {

void click_idle(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class, 0), LV_EVENT_CLICKED, nullptr);
}

void click_running(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class, 1), LV_EVENT_CLICKED, nullptr);
}

// The two scrolling labels move: compare them while they do.
void after_one_second(lv::obj) {
  pass_time(1000);
}

void after_four_seconds(lv::obj) {
  pass_time(4000);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("label_bind_text", &lv_example_label_bind_text, &widgets::label_bind_text, &nothing);
  same_pixels("label_bind_text running", &lv_example_label_bind_text, &widgets::label_bind_text,
              &click_running);
  same_pixels("label_bind_text idle", &lv_example_label_bind_text, &widgets::label_bind_text,
              &click_idle);
  same_pixels("label_long_mode", &lv_example_label_long_mode, &widgets::label_long_mode, &nothing);
  same_pixels("label_long_mode 1 s", &lv_example_label_long_mode, &widgets::label_long_mode,
              &after_one_second);
  same_pixels("label_long_mode 4 s", &lv_example_label_long_mode, &widgets::label_long_mode,
              &after_four_seconds);
  same_pixels("label_recolor", &lv_example_label_recolor, &widgets::label_recolor, &nothing);
  same_pixels("label_set_text_newline", &lv_example_label_set_text_newline,
              &widgets::label_set_text_newline, &nothing);
  same_pixels("label_styling", &lv_example_label_styling, &widgets::label_styling, &nothing);
  same_pixels("label_text_align", &lv_example_label_text_align, &widgets::label_text_align,
              &nothing);

  // A different picture must be noticed, and so must a label that moved.
  const auto still = render(&widgets::label_long_mode, &nothing);
  const auto moved = render(&widgets::label_long_mode, &after_one_second);
  check(still.first_difference(moved) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
