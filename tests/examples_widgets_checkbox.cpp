// The checkbox examples of LVGL's examples/widgets, rewritten with lvgl26, must
// render the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_checkbox_bind_checked(void);
void lv_example_checkbox_event(void);
void lv_example_checkbox_states(void);
void lv_example_checkbox_styling(void);
void lv_example_checkbox_text(void);
}

namespace {

// What a tap does: the box changes its state and tells its listeners. Then wait
// until the theme's transition is over.
void tick(lv::obj root, bool on) {
  lv_obj_t* box = nth(root, &lv_checkbox_class);
  lv_obj_set_state(box, LV_STATE_CHECKED, on);
  lv_obj_send_event(box, LV_EVENT_VALUE_CHANGED, nullptr);
  pass_time(500);
}

void tick_it(lv::obj root) { tick(root, true); }

void tick_and_untick(lv::obj root) {
  tick(root, true);
  tick(root, false);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("checkbox_bind_checked", &lv_example_checkbox_bind_checked,
              &widgets::checkbox_bind_checked, &nothing);
  same_pixels("checkbox_bind_checked on", &lv_example_checkbox_bind_checked,
              &widgets::checkbox_bind_checked, &tick_it);
  same_pixels("checkbox_bind_checked off", &lv_example_checkbox_bind_checked,
              &widgets::checkbox_bind_checked, &tick_and_untick);
  same_pixels("checkbox_event", &lv_example_checkbox_event, &widgets::checkbox_event, &nothing);
  same_pixels("checkbox_event on", &lv_example_checkbox_event, &widgets::checkbox_event, &tick_it);
  same_pixels("checkbox_event off again", &lv_example_checkbox_event, &widgets::checkbox_event,
              &tick_and_untick);
  same_pixels("checkbox_states", &lv_example_checkbox_states, &widgets::checkbox_states, &nothing);
  same_pixels("checkbox_styling", &lv_example_checkbox_styling, &widgets::checkbox_styling, &nothing);
  same_pixels("checkbox_styling ticked", &lv_example_checkbox_styling, &widgets::checkbox_styling,
              &tick_it);
  same_pixels("checkbox_text", &lv_example_checkbox_text, &widgets::checkbox_text, &nothing);

  // The note must appear with the tick.
  const auto without = render(&widgets::checkbox_bind_checked, &tick_and_untick);
  const auto with = render(&widgets::checkbox_bind_checked, &tick_it);
  check(without.first_difference(with) != -1);

  // The ticked box has a look of its own.
  const auto plain = render(&widgets::checkbox_styling, &nothing);
  const auto ticked = render(&widgets::checkbox_styling, &tick_it);
  check(plain.first_difference(ticked) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
