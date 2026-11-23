// What the SDL window does when it is closed, hidden and shown, run with SDL's dummy video
// driver: a test of examples/platform/sdl_window.hpp, built with the window examples.
// editor-check: skip, it needs SDL2, which the window examples find in CMake

#include <lv/lv.hpp>
#include <lv/pointer.hpp>
#include <sdl_window.hpp>

#include <cstdio>
#include <cstdlib>

namespace {

using namespace lv::units;

constinit lv::frame<160, 120> frame;
constexpr auto ui =
  lv::column({.size = {160 * px, 120 * px}}, lv::label({.id = "text", .text = "Window"}));

int failures = 0;
void check(bool ok, int line) {
  if (ok) return;
  ++failures;
  std::printf("behaviour.cpp:%d: check failed\n", line);
}

void request_close() {
  SDL_Event e{};
  e.type = SDL_QUIT;
  SDL_PushEvent(&e);
}

platform::close hide_it() { return platform::close::hide; }
platform::close keep_it() { return platform::close::ignore; }

// A close request the program ignores changes nothing; one it turns into hiding keeps the
// tree and its state, and the window shows them again.
template <class Window, class Root>
void close_taken(lv::application& app, Window& window, lv::display& display, Root& root) {
  window.on_close(&keep_it);
  request_close();
  app.step();
  check(window.shown() && display.visible(), __LINE__);
  window.on_close(&hide_it);
  request_close();
  app.step();
  check(!window.shown() && !display.visible() && root->root(), __LINE__);
  window.show();
  check(
    window.shown() && display.visible() &&
      root->template get<"text">().template get<"text">() != nullptr,
    __LINE__
  );
}

// display.hide() and display.show() ask the window to hide and show itself.
template <class Window>
void display_asks_the_window(Window& window, lv::display& display) {
  display.hide();
  check(!window.shown() && !display.visible(), __LINE__);
  display.show();
  check(window.shown() && display.visible(), __LINE__);
}

}  // namespace

int main() {
  setenv("SDL_VIDEODRIVER", "dummy", 0);
  lv::application app;
  platform::sdl_window window{app, "behaviour", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};
  lv::pointer mouse{display, window};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 2;
  app.advance(40 * ms), app.step();

  close_taken(app, window, display, root);
  display_asks_the_window(window, display);

  // By default a close request ends run().
  window.on_close(nullptr);
  request_close();
  check(app.run() == 0, __LINE__);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
