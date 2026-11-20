// A screen of ten widget kinds, mounted: what each further kind costs the compiler.
#include <lv/lv.hpp>

constexpr auto ui = lv::column({})[
    lv::label({.text = "Hello"}),
    lv::button({})[lv::label({.text = "Go"})],
    lv::arc({.range = {0, 100}}),
    lv::slider({.range = {0, 100}}),
    lv::bar({.range = {0, 100}}),
    lv::toggle({}),
    lv::checkbox({.text = "Yes"}),
    lv::roller({}),
    lv::spinner({}),
    lv::textarea({})];

constinit lv::frame<320, 240> frame;

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
