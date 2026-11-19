// The application of app.hpp in a desktop window. It needs SDL2: cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON.
// editor-check: skip, it needs SDL2, which the window examples find in CMake
//
// The platform is sdl_window: a window that shows the frame, and a mouse that feeds the
// pointer. docs/input.md shows this file.

#include "app.hpp"

#include <lv/host.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

namespace {

constinit lv::frame<demo::width, demo::height> frame;

}  // namespace

//! [main]
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{"lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display};               // the window's mouse feeds it

  auto root = lv::mount<demo::ui>(display.screen());
  if (!root) return 2;
  lv::spawn(demo::tick());
  while (window.feed(mouse)) window.wait(app.step());
  return 0;
}
//! [main]
