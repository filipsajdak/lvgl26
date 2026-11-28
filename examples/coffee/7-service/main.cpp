// Step 7 of the coffee machine tutorial (docs/research/tutorial-coffee-draft.md): a service page in
// a file of its own, and the program's settings in lvgl26.json.
//
//   coffee_7    writes coffee_7.png, the finished machine as it starts, and coffee_7_service.png,
//               its service page twelve cups later
//
// main.cpp starts the application and draws the machine without a window, as the steps before it
// do. platform/window.cpp is the same machine in a window.

#include "design/theme.hpp"
#include "shell.hpp"
#include "state.hpp"

#include <lv/lv.hpp>
#include <lv/png.hpp>

namespace {

constinit lv::frame<coffee::width, coffee::height> frame;

}  // namespace

//! [main]
int main() {
  lv::application app;
  lv::display display{app, frame, coffee::theme};
  if (!coffee::start(display.screen())) return 1;
  //! [main]

  // The finished machine, as it starts.
  for (int i = 0; i < 10; ++i) app.advance(10 * lv::ms), app.step();
  if (!lv::write_png(display.screen(), "coffee_7.png")) return 2;

  // Twelve cups later, the service page, once it has slid in.
  coffee::cups    = lv::counter{12};
  coffee::current = coffee::step::service;
  for (int i = 0; i < 50; ++i) app.advance(10 * lv::ms), app.step();

  return lv::write_png(display.screen(), "coffee_7_service.png") ? 0 : 3;
}
