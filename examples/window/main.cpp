// An application of examples/wasm in a desktop window. It needs SDL2: cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON.
// editor-check: skip, it needs SDL2, and includes app.hpp from the directory of the application it is built with
//
// The same application runs in the browser (examples/wasm/platform.cpp) and here: each platform
// gives it an lv::display and an lv::pointer. Here the platform is sdl_window, a window that
// shows the frame, with a mouse that feeds the pointer. docs/input.md shows this file.

#include "app.hpp"  // the application: counter/app.hpp or bounce/app.hpp, by the include path

#include <lv/host.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

namespace {

constinit lv::frame<app::width, app::height> frame;

}  // namespace

//! [main]
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{"lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display};               // the window's mouse feeds it

  app::running application{display.screen()};
  if (!application.ok()) return 2;
  while (window.feed(mouse)) window.wait(app.step());
  return 0;
}
//! [main]
