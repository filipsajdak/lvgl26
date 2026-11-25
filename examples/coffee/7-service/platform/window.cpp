// coffee_7_window: the coffee machine in a window, the only file that knows the platform. It
// needs SDL2 (cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON).
// editor-check: skip, it needs SDL2, which the window examples find in CMake

#include "../design/theme.hpp"
#include "../shell.hpp"

#include <lv/host.hpp>
#include <lv/lv.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

namespace {

constinit lv::frame<coffee::width, coffee::height> frame;

}  // namespace

int main() {
  lv::application app{lv::host};
  platform::sdl_window window{app, "coffee", frame};
  if (!window) return 1;
  lv::display display{app, frame, window, coffee::theme};
  lv::pointer mouse{display, window};
  if (!coffee::start(display.screen())) return 2;
  return app.run();  // until the window is closed
}
