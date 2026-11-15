// The arc examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_arc_bind_value(void);
void lv_example_arc_change_rate(void);
void lv_example_arc_event(void);
void lv_example_arc_img_indicator(void);
void lv_example_arc_mode(void);
void lv_example_arc_pie_chart(void);
void lv_example_arc_rotation(void);
void lv_example_arc_set_angles(void);
void lv_example_arc_styling(void);
void lv_example_arc_value_range(void);
}

namespace {

// What a drag does: the widget gets a new value and tells its listeners.
void drag_arc_to(lv::obj root, std::int32_t value) {
  lv_obj_t* arc = nth(root, &lv_arc_class);
  lv_arc_set_value(arc, value);
  lv_obj_send_event(arc, LV_EVENT_VALUE_CHANGED, nullptr);
}

void slide_to_80(lv::obj root) {
  lv_obj_t* slider = nth(root, &lv_slider_class);
  lv_slider_set_value(slider, 80, false);
  lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, nullptr);
}

void drag_to_30(lv::obj root) { drag_arc_to(root, 30); }
void drag_to_7(lv::obj root) { drag_arc_to(root, 7); }
void drag_to_90(lv::obj root) { drag_arc_to(root, 90); }

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("arc_bind_value", &lv_example_arc_bind_value, &widgets::arc_bind_value, &nothing);
  same_pixels("arc_bind_value at 80", &lv_example_arc_bind_value, &widgets::arc_bind_value,
              &slide_to_80);
  same_pixels("arc_change_rate", &lv_example_arc_change_rate, &widgets::arc_change_rate, &nothing);
  same_pixels("arc_event", &lv_example_arc_event, &widgets::arc_event, &nothing);
  same_pixels("arc_event at 30", &lv_example_arc_event, &widgets::arc_event, &drag_to_30);
  same_pixels("arc_img_indicator", &lv_example_arc_img_indicator, &widgets::arc_img_indicator,
              &nothing);
  same_pixels("arc_img_indicator at 7", &lv_example_arc_img_indicator,
              &widgets::arc_img_indicator, &drag_to_7);
  same_pixels("arc_mode", &lv_example_arc_mode, &widgets::arc_mode, &nothing);
  same_pixels("arc_pie_chart", &lv_example_arc_pie_chart, &widgets::arc_pie_chart, &nothing);
  same_pixels("arc_rotation", &lv_example_arc_rotation, &widgets::arc_rotation, &nothing);
  same_pixels("arc_set_angles", &lv_example_arc_set_angles, &widgets::arc_set_angles, &nothing);
  same_pixels("arc_styling", &lv_example_arc_styling, &widgets::arc_styling, &nothing);
  same_pixels("arc_value_range", &lv_example_arc_value_range, &widgets::arc_value_range, &nothing);
  // Inside 0..100 only: past 100 the two differ, as arc_value_range.cpp says.
  same_pixels("arc_value_range at 90", &lv_example_arc_value_range, &widgets::arc_value_range,
              &drag_to_90);

  // A new value must be a different picture.
  const auto before = render(&widgets::arc_img_indicator, &drag_to_7);
  const auto after = render(&widgets::arc_img_indicator, &drag_to_30);
  check(before.first_difference(after) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
