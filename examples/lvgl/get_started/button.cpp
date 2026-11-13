#include "get_started.hpp"

namespace {

using namespace lv::units;

// How often the button was clicked. The label follows it.
// One difference from the C example: that one shows "Button" until the first
// click, this one shows "Button: 0".
constinit lv::state clicks{lv::counter{}};

void count_click() {
  clicks.update([](lv::counter c) { return ++c; });
}

constexpr auto ui = lv::group[
    lv::button({.on_clicked = &count_click, .pos = {10 * px, 10 * px}, .size = {120 * px, 50 * px}})[
        lv::label({.align = LV_ALIGN_CENTER, .text = lv::bind(clicks, "Button: {}")})]];

}  // namespace

void get_started::button(lv::obj screen) {
  if (auto mounted = lv::mount<ui>(screen)) mounted->detach();
}
