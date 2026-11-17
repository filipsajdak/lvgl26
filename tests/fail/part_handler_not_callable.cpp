#include <lv/lv.hpp>
constexpr auto tap_button(lv::event_handler auto clicked) { return lv::button({.on_clicked = clicked}); }
constexpr auto ui = tap_button(42);  // a number is no handler
