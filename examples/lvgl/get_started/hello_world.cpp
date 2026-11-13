#include "get_started.hpp"

namespace {

using namespace get_started;

// The C example styles the screen itself. Here a group that fills the screen
// carries the background and the text color, which the label inherits.
constexpr auto ui = lv::group({.style = {.bg_color = tok.screen.background,
                                         .bg_opa = tok.opacity.opaque,
                                         .text_color = tok.screen.text}})[
    lv::label({.align = LV_ALIGN_CENTER, .text = "Hello world"})];

}  // namespace

void get_started::hello_world(lv::obj screen) {
  if (auto mounted = lv::mount<ui>(screen)) mounted->detach();
}
