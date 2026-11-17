// printer_shot <directory>: writes each page of the printer panel into the directory as a PNG file.

#include "scenes.hpp"
#include "shell.hpp"

#include <lv/png.hpp>

#include <cstdio>
#include <string>

namespace {

constinit lv::frame<printer::window_width, printer::window_height> frame;

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::puts("usage: printer_shot <directory>");
    return 2;
  }
  lv::application app;
  lv::display display{app, frame};
  if (!printer::start(display.screen())) return 1;
  for (const printer::scene& s : printer::scenes) {
    printer::play(app, s);
    if (!lv::write_png(display.screen(), (std::string{argv[1]} + "/" + s.name + ".png").c_str())) return 1;
  }
  return 0;
}
