// bounce_shot <directory> <ms>...: runs scene.hpp on a clock that steps 10 ms at a time, and
// writes the screen at each of the given times into the directory as bounce_<ms>.png. The test
// example_bounce compares some of them with tests/ref; a recording of the site uses many.

#include "scene.hpp"

#include <lv/png.hpp>

#include <cstdlib>
#include <string>

namespace {

constinit lv::frame<bounce::screen_width, bounce::screen_height> frame;
constinit lv::mounted<bounce::scene> page;

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  lv::application app;
  lv::display display{app, frame};
  if (!bounce::start(page, display.screen())) return 1;
  int now = 0;
  for (int i = 2; i < argc; ++i) {
    const int at = std::atoi(argv[i]);
    for (; now < at; now += bounce::frame_ms) {
      app.advance(bounce::frame_time);
      app.step();
    }
    const std::string path = std::string{argv[1]} + "/bounce_" + argv[i] + ".png";
    if (!lv::write_png(display.screen(), path.c_str())) return 1;
  }
  return 0;
}
