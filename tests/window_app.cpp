// The counter of examples/wasm, driven as its window and its browser page drive it: a frame,
// a display and an lv::pointer fed with a mouse's events. The same source runs in the window,
// in the page, here, and on a device.
#include "../examples/wasm/counter/app.hpp"

#include <lv/pointer.hpp>

#include "check.hpp"

#include <cstdint>

namespace {

using namespace lv::units;

constinit lv::frame<app::width, app::height> frame;

// The states the counter shows, as app::state numbers them.
constexpr std::int32_t dial = 0;
constexpr std::int32_t clicks = 1;
constexpr std::int32_t ticks = 2;
constexpr std::int32_t volume = 3;

// The widget at `path` from the screen: child number path[0], then its child path[1], ...
lv_obj_t* at_path(lv::obj screen, std::initializer_list<std::int32_t> path) {
  lv_obj_t* w = screen.raw();
  for (std::int32_t i : path) w = lv_obj_get_child(w, i);
  return w;
}

lv::point middle(lv_obj_t* widget) {
  lv_area_t at;
  lv_obj_get_coords(widget, &at);
  return {(at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2};
}

// A step of 16 ms, as a window at 60 frames a second.
void frame_step(lv::application& run) { run.advance(16 * ms), run.step(); }

// Two quick clicks on "+", each shorter than one read period of LVGL.
void two_clicks(lv::application& run, lv::pointer<>& mouse, lv::obj screen) {
  const lv::point more = middle(at_path(screen, {0, 2, 2}));  // column, the row of buttons, "+"
  check(mouse.press(more) && mouse.release() && mouse.press(more) && mouse.release());
  frame_step(run);
  check(app::state(dial) == 2 * demo::step_of_dial && app::state(clicks) == 2);
}

// A drag of the slider from its left end to its right end, as mouse moves.
void drag(lv::application& run, lv::pointer<>& mouse, lv::obj screen) {
  lv_area_t at;
  lv_obj_get_coords(at_path(screen, {0, 3, 0}), &at);  // column, the last row, the slider
  const std::int32_t y = (at.y1 + at.y2) / 2;
  bool kept = mouse.press({at.x1 + 2, y});
  for (std::int32_t x = at.x1 + 2; x <= at.x2; x += 20) kept = mouse.move({x, y}) && kept;
  kept = mouse.move({at.x2 - 2, y}) && mouse.release() && kept;
  frame_step(run);
  check(kept && app::state(volume) > 90);
}

}  // namespace

int main() {
  lv::application run;
  lv::display display{run, frame};
  lv::pointer mouse{display};
  app::running application{display.screen()};
  if (!application.ok()) return 1;
  frame_step(run);

  two_clicks(run, mouse, display.screen());
  drag(run, mouse, display.screen());

  // The task ticks on its own.
  for (int i = 0; i < 20; ++i) frame_step(run);
  check(app::state(ticks) >= 1);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
