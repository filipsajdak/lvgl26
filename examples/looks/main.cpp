// The screen of examples/looks/screen.hpp, drawn. docs/looks.md walks through it.
//
//   looks    writes looks.png

#include "screen.hpp"

#include <lv/png.hpp>

namespace {
constinit lv::frame<320, 240> frame;
}

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<looks::ui>(display.screen());
  return root && lv::write_png(display.screen(), "looks.png") ? 0 : 1;
}
