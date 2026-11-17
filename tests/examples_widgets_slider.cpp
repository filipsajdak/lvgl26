// The slider examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_slider_bind_state(void);
void lv_example_slider_bind_value(void);
void lv_example_slider_event(void);
void lv_example_slider_img_indicator(void);
void lv_example_slider_modes(void);
void lv_example_slider_reversed_direction(void);
void lv_example_slider_value_range_orientation(void);
}

namespace {

// What a drag does: the slider gets a new value and tells its listeners.
void drag_to(lv::obj root, std::int32_t value) {
  lv_obj_t* slider = nth(root, &lv_slider_class);
  lv_slider_set_value(slider, value, false);
  lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, nullptr);
}

void drag_to_7(lv::obj root) { drag_to(root, 7); }
void drag_to_30(lv::obj root) { drag_to(root, 30); }
void drag_to_80(lv::obj root) { drag_to(root, 80); }

// Above 80 the button is disabled. Its look changes with a transition, so wait.
void drag_to_81(lv::obj root) {
  drag_to(root, 81);
  pass_time(500);
}

void drag_and_let_go(lv::obj root) {
  drag_to(root, 40);
  drag_to(root, 60);
  lv_obj_send_event(nth(root, &lv_slider_class), LV_EVENT_RELEASED, nullptr);
}

// Let go, then dragged again: the label goes back to "Dragging".
void drag_again(lv::obj root) {
  drag_and_let_go(root);
  drag_to(root, 20);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("slider_bind_state", &lv_example_slider_bind_state, &widgets::slider_bind_state,
              &nothing);
  same_pixels("slider_bind_state at 81", &lv_example_slider_bind_state,
              &widgets::slider_bind_state, &drag_to_81);
  // Built again while the value is above 80: the button starts disabled.
  same_pixels("slider_bind_state rebuilt", &lv_example_slider_bind_state,
              &widgets::slider_bind_state, &nothing);
  same_pixels("slider_bind_state at 80", &lv_example_slider_bind_state,
              &widgets::slider_bind_state, &drag_to_80);
  same_pixels("slider_bind_value", &lv_example_slider_bind_value, &widgets::slider_bind_value,
              &nothing);
  same_pixels("slider_bind_value at 30", &lv_example_slider_bind_value,
              &widgets::slider_bind_value, &drag_to_30);
  same_pixels("slider_event", &lv_example_slider_event, &widgets::slider_event, &nothing);
  same_pixels("slider_event dragging", &lv_example_slider_event, &widgets::slider_event,
              &drag_to_30);
  same_pixels("slider_event released", &lv_example_slider_event, &widgets::slider_event,
              &drag_and_let_go);
  same_pixels("slider_event again", &lv_example_slider_event, &widgets::slider_event, &drag_again);
  same_pixels("slider_img_indicator", &lv_example_slider_img_indicator,
              &widgets::slider_img_indicator, &nothing);
  same_pixels("slider_img_indicator at 7", &lv_example_slider_img_indicator,
              &widgets::slider_img_indicator, &drag_to_7);
  same_pixels("slider_modes", &lv_example_slider_modes, &widgets::slider_modes, &nothing);
  same_pixels("slider_reversed_direction", &lv_example_slider_reversed_direction,
              &widgets::slider_reversed_direction, &nothing);
  same_pixels("slider_value_range_orient.", &lv_example_slider_value_range_orientation,
              &widgets::slider_value_range_orientation, &nothing);

  // Dragging and letting go must be different pictures.
  const auto dragging = render(&widgets::slider_event, &drag_to_30);
  const auto released = render(&widgets::slider_event, &drag_and_let_go);
  check(dragging.first_difference(released) != -1);
  // So must a disabled and an enabled button.
  const auto enabled = render(&widgets::slider_bind_state, &drag_to_80);
  const auto disabled = render(&widgets::slider_bind_state, &drag_to_81);
  check(enabled.first_difference(disabled) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
