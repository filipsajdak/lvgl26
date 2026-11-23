#include "get_started.hpp"

namespace {

using namespace lv::units;

// How often the button was clicked. The label follows it.
// One difference from the C example: that one shows "Button" until the first
// click, this one shows "Button: 0".
constinit lv::state clicks{lv::counter{}};

void count_click() { ++clicks; }

constexpr auto ui = lv::group(
  lv::button(
    {
      .pos        = {10 * px, 10 * px},
      .size       = {120 * px, 50 * px},
      .on_clicked = &count_click,
    },
    lv::label({.align = "center", .text = lv::bind(clicks, "Button: {}")})
  )
);

}  // namespace

void get_started::button(lv::obj screen) { lv::place<ui>(screen); }
