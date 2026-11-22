// The switch examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_switch_event(void);
void lv_example_switch_orientation(void);
void lv_example_switch_state(void);
void lv_example_switch_styling(void);
}

namespace {

// What a tap does: the switch changes its state and tells its listeners. Then
// wait until the knob has moved.
void turn(lv::obj root, bool on) {
  lv_obj_t* toggle = nth(root, &lv_switch_class);
  lv_obj_set_state(toggle, LV_STATE_CHECKED, on);
  lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
  pass_time(500);
}

void turn_on(lv::obj root) { turn(root, true); }

void turn_on_and_off(lv::obj root) {
  turn(root, true);
  turn(root, false);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("switch_event", &lv_example_switch_event, &widgets::switch_event, &nothing);
  same_pixels("switch_event on", &lv_example_switch_event, &widgets::switch_event, &turn_on);
  same_pixels("switch_event off again", &lv_example_switch_event, &widgets::switch_event,
              &turn_on_and_off);
  same_pixels("switch_orientation", &lv_example_switch_orientation, &widgets::switch_orientation,
              &nothing);
  same_pixels("switch_state", &lv_example_switch_state, &widgets::switch_state, &nothing);
  same_pixels("switch_styling", &lv_example_switch_styling, &widgets::switch_styling, &nothing);
  same_pixels("switch_styling on", &lv_example_switch_styling, &widgets::switch_styling, &turn_on);

  // On and off must be different pictures.
  const auto off = render(&widgets::switch_event, &turn_on_and_off);
  const auto on = render(&widgets::switch_event, &turn_on);
  check(off.first_difference(on) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
