// A screen of one label, mounted: what the first widget of a program costs the compiler.
#include <lv/lv.hpp>

constexpr auto ui = lv::label({.text = "Hello"});

constinit lv::frame<320, 240> frame;

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
