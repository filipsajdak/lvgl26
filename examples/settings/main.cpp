// A program that each settings file in presets/ builds: it uses none of what they forbid.
#include <lv/lv.hpp>

constinit lv::state taps{lv::counter{}};
void tapped() { ++taps; }

constexpr auto ui = lv::column[
    lv::label({.text = lv::bind(taps, "{} taps")}),
    lv::button({.on_clicked = &tapped})[lv::label({.text = "Tap"})]];

constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  ++taps;
  return taps.get().value == 1 ? 0 : 2;
}
