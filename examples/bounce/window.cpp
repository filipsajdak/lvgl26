// bounce: the bouncing ball of scene.hpp in a window. It needs SDL2 (cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON).
// editor-check: skip, it calls LVGL's SDL driver, which exists only when the window option is on

#include "scene.hpp"

#include <lv/host.hpp>

namespace {
constinit lv::mounted<bounce::scene> page;
}

int main() {
  lv::application app{lv::host};
  lv_sdl_window_create(bounce::screen_width, bounce::screen_height);  // C API: no window display in lv:: yet
  if (!bounce::start(page, lv::screen_active())) return 1;
  return app.run();
}
