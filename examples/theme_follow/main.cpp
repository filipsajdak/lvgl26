// A display that follows a state: Signal's day and dark, flipped by a row of the screen.
// docs/themes.md walks through this file.
//
//   theme_follow    writes theme_follow_day.png and theme_follow_dark.png

#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/signal.hpp>

using namespace lv::units;

namespace {

//! [mode]
constinit lv::state mode{lv::signal::mode::day};
//! [mode]

//! [tree]
constexpr auto service = lv::column(
  {.size = {320 * px, 240 * px}}, lv::label({.text = "Service"}),
  lv::row(
    lv::label({.text = "Display"}), lv::dropdown({
                                      .options  = lv::options_of<lv::signal::mode>,
                                      .selected = lv::bind(mode),
                                    })
  ),
  lv::button({.id = "start"}, lv::label({.text = "Start"})),
  lv::arc({.id = "ring", .size = {80 * px, 80 * px}, .value = 60})
);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  //! [follow]
  display.follow(mode, {lv::signal::day, lv::signal::dark});  // day first, as the enum has it
  //! [follow]
  auto root = lv::mount<service>(display.screen());
  if (!root) return 1;
  app.step();
  if (!lv::write_png(display.screen(), "theme_follow_day.png")) return 2;
  //! [flip]
  mode = lv::signal::mode::dark;  // every widget of the display changes in place
  //! [flip]
  app.step();
  return lv::write_png(display.screen(), "theme_follow_dark.png") ? 0 : 3;
}
