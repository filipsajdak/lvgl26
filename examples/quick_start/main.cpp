// The quick start of the documentation: docs/quick-start.md shows this file piece by piece.

#include <lv/all.hpp>
#include <lv/png.hpp>
#include <lv/signal.hpp>

#include <mp-units/systems/si.h>

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

//! [readme]
//! [state]
constinit lv::state speed{0 * km / h};
//! [state]

//! [tree]
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::label({.text = "Speed"}),
  lv::arc(
    {
      .range = {0, 240},
      .size  = {120 * px, 120 * px},
      .value = lv::bind(speed),
    },
    lv::label({.align = "center", .text = lv::bind(speed)})
  ),
  lv::button({.on_clicked = [] { speed = 0 * km / h; }}, lv::label({.text = "Reset"}))
);
//! [tree]
//! [readme]

//! [main]
constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;                                // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame, lv::signal::dark};  // no panel; Signal is the look

  auto root = lv::mount<ui>(display.screen());  // creates the widgets, and owns them
  if (!root) return 1;

  speed = 120 * km / h;  // the dial and the text follow
  return lv::write_png(display.screen(), "quick_start.png") ? 0 : 2;
}
//! [main]
