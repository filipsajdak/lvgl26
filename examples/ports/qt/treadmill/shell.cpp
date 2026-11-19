// The window of the panel: where the main screen is shown, where the splash screen is shown over
// it, and what changes the main screen when the look changes.

#include "pages.hpp"
#include "shell.hpp"
#include "state.hpp"
#include "ui.hpp"

namespace treadmill {
namespace {

constexpr auto ui = lv::group({.size = {screen_width * px, screen_height * px}})[
    lv::group({.id = "main", .size = {screen_width * px, screen_height * px}}),
    lv::group({.id = "splash", .size = {screen_width * px, screen_height * px}})];

constexpr lv::duration splash_time = 5000 * ms;
constexpr lv::duration hand_over = 1 * ms;  // a page is not deleted inside the click that asked for it

constexpr make_page main_maker(look l) {
  switch (l) {
    case look::light: return &mount_main_light;
    case look::dark: return &mount_main_dark;
    case look::sporty: return &mount_main_sporty;
  }
  return &mount_main_light;
}

lv::owned<lv::obj> main_page;
lv::owned<lv::obj> splash_page;

// The look is a state. A new look is a new tree of the same screen, built after the click.
lv::task<> follow_look(lv::obj host) {
  main_page = main_maker(active_look.get())(host);
  for (;;) {
    co_await lv::changed(active_look);
    co_await lv::sleep(hand_over);
    main_page.reset();
    main_page = main_maker(active_look.get())(host);
  }
}

lv::task<> remove_splash(lv::obj host) {
  splash_page = mount_splash(host);
  co_await lv::sleep(splash_time);
  splash_page.reset();
  host.set({.hidden = true});
}

}  // namespace

bool start(lv::obj screen) {
  static auto window = lv::mount<ui>(screen);
  if (!window) return false;
  lv::spawn(follow_look(window->get<"main">()), window->get<"main">());
  lv::spawn(remove_splash(window->get<"splash">()), window->get<"splash">());
  // The workout clock and the automatic mode belong to the program, not to a screen: they
  // run as long as it does, so they have no owner.
  lv::spawn(run_clock());
  lv::spawn(run_auto());
  return start_readouts();
}

}  // namespace treadmill
