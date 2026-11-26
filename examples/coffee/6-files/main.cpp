// Step 6 of the coffee machine tutorial (docs/research/tutorial-coffee.md): the program of step 5,
// split into files by screen. Nothing else changes, and the picture is the one of step 5.
//
//   coffee_6    writes coffee_6.png: the page of your cup
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

  // The page of your cup, once it has slid in.
  coffee::current = coffee::step::cup;
  for (int i = 0; i < 50; ++i) app.advance(10 * lv::ms), app.step();

  return lv::write_png(display.screen(), "coffee_6.png") ? 0 : 2;
}
