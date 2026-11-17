// treadmill: the treadmill panel in a window. It needs SDL2 (cmake -DLVGL26_BUILD_WINDOW_EXAMPLES=ON).
// editor-check: skip, it calls LVGL's SDL driver, which exists only when the window option is on

#include "shell.hpp"

#include <lv/host.hpp>

int main() {
  lv::application app{lv::host};
  lv_sdl_window_create(treadmill::screen_width, treadmill::screen_height);  // C API: no window display in lv:: yet
  lv_sdl_mouse_create();                                                    // C API
  if (!treadmill::start(lv::screen_active())) return 1;
  return app.run();
}
