#include <lv/lv.hpp>
// A part that builds a constexpr tree takes a lv::tree_handler: the capture is refused at its call.
int taps = 0;
constexpr auto tap_button(lv::tree_handler auto clicked) { return lv::button({.on_clicked = clicked}); }
constexpr auto make(int n) { return tap_button([n] { taps += n; }); }
constexpr auto ui = make(1);
