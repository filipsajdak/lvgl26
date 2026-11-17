#include <lv/lv.hpp>
// The part accepts any event handler, and the capture is refused where the tree takes it.
int taps = 0;
constexpr auto tap_button(lv::event_handler auto clicked) { return lv::button({.on_clicked = clicked}); }
constexpr auto make(int n) { return tap_button([n] { taps += n; }); }
constexpr auto ui = make(1);
