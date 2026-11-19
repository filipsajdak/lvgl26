// bounce: the bouncing ball of scene.hpp in a window. It needs SDL2 (cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON).
// editor-check: skip, it needs SDL2, which the window examples find in CMake

#include "scene.hpp"

#include <lv/host.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

namespace {
constinit lv::frame<bounce::screen_width, bounce::screen_height> frame;
constinit lv::mounted<bounce::scene> page;
}  // namespace

int main() {
  lv::application app{lv::host};
  platform::sdl_window window{"bounce", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};
  lv::pointer mouse{display};
  if (!bounce::start(page, display.screen())) return 2;
  while (window.feed(mouse)) window.wait(app.step());
  return 0;
}
