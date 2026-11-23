// Pictures from PNG files, decoded by the compiler. docs/images.md walks through this file.
//
//   images    writes images.png

#include <lv/image.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

using namespace lv::units;

namespace {

//! [files]
constexpr unsigned char sky_png[] = {
#embed "sky.png"
};
constexpr unsigned char sun_png[] = {
#embed "sun.png"
};

constexpr auto sky =
  lv::image_file<sky_png>;  // every pixel opaque: stored as the display's own pixels
constexpr auto sun =
  lv::image_file<sun_png>;  // see-through around the sun: stored with an alpha plane
//! [files]

//! [tree]
constexpr auto ui = lv::box(
  {.size = {320 * px, 240 * px}}, lv::image({.align = "center", .src = sky}),
  lv::image({.align = "center", .src = sun}),
  lv::label({.align = "bottom_mid", .text = "Two pictures, no decoder on the device"})
);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "images.png") ? 0 : 2;
}
