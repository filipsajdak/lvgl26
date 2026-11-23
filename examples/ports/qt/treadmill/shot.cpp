// treadmill_shot <directory>: writes each scene of the treadmill panel into the directory as a PNG file.

#include "scenes.hpp"

#include <lv/png.hpp>

#include <cstdio>
#include <string>

namespace {

constinit lv::frame<treadmill::screen_width, treadmill::screen_height> frame;

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::puts("usage: treadmill_shot <directory>");
    return 2;
  }
  lv::application app;
  lv::display display{app, frame};
  if (!treadmill::start(display.screen())) return 1;
  for (const treadmill::scene& s : treadmill::scenes) {
    treadmill::play(app, s);
    if (!lv::write_png(display.screen(), (std::string{argv[1]} + "/" + s.name + ".png").c_str()))
      return 1;
  }
  return 0;
}
