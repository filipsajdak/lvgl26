// The header that includes everything of lvgl26 that is not a platform or a tool. It declares nothing of
// its own, so the test is that a file which includes it compiles and runs a screen: a damaged line in
// it, or a name that two of its headers declare, stops this file.
#include <lv/all.hpp>

namespace {

constexpr auto ui = lv::label({.text = "all"});
constinit lv::frame<64, 48> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  return root ? 0 : 1;
}
