// The application of examples/window, driven as its window drives it: a frame, a display
// and an lv::pointer fed with a mouse's events, without SDL. The same source runs in the
// window, here, and on a device.
#include "../examples/window/app.hpp"

#include <lv/pointer.hpp>

#include "check.hpp"

#include <cstdint>

namespace {

using namespace lv::units;

constinit lv::frame<demo::width, demo::height> frame;

lv::point middle(lv::obj widget) {
  lv_area_t at;
  lv_obj_get_coords(widget.raw(), &at);
  return {(at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2};
}

// A step of 16 ms, as a window at 60 frames a second.
void frame_step(lv::application& app) { app.advance(16 * ms), app.step(); }

// Two quick clicks on "+", each shorter than one read period of LVGL.
void two_clicks(lv::application& app, lv::pointer<>& mouse, lv::obj more) {
  const lv::point at = middle(more);
  check(mouse.press(at) && mouse.release() && mouse.press(at) && mouse.release());
  frame_step(app);
  check(demo::dial.get().value == 2 * demo::step_of_dial && demo::clicks.get().value == 2);
}

// A drag of the slider from its left end to its right end, as mouse moves.
void drag(lv::application& app, lv::pointer<>& mouse, lv::obj volume) {
  lv_area_t at;
  lv_obj_get_coords(volume.raw(), &at);
  const std::int32_t y = (at.y1 + at.y2) / 2;
  bool kept = mouse.press({at.x1 + 2, y});
  for (std::int32_t x = at.x1 + 2; x <= at.x2; x += 20) kept = mouse.move({x, y}) && kept;
  kept = mouse.move({at.x2 - 2, y}) && mouse.release() && kept;
  frame_step(app);
  check(kept && demo::volume.get().value > 90);
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  lv::pointer mouse{display};
  auto root = lv::mount<demo::ui>(display.screen());
  if (!root) return 1;
  lv::spawn(demo::tick());
  frame_step(app);

  two_clicks(app, mouse, root->get<"more">());
  drag(app, mouse, root->get<"volume">());

  // The task ticks on its own.
  for (int i = 0; i < 20; ++i) frame_step(app);
  check(demo::ticks.get().value >= 1);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
