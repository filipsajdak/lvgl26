// treadmill: the treadmill panel in a window. It needs SDL2 (cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON).
// editor-check: skip, it needs SDL2, which the window examples find in CMake

#include "shell.hpp"
#include "sizes.hpp"

#include <lv/host.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

namespace {

constinit lv::frame<treadmill::screen_width, treadmill::screen_height> frame;

}  // namespace

int main() {
  lv::application app{lv::host};
  platform::sdl_window window{app, "treadmill", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};
  lv::pointer mouse{display, window};
  if (!treadmill::start(display.screen())) return 2;
  return app.run();                         // until the window is closed
}
