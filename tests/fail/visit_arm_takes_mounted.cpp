#include <lv/lv.hpp>
constexpr auto home = lv::column[lv::label({.text = "Home"})];
constexpr auto settings = lv::column[lv::label({.text = "Settings"})];
constinit lv::one_of<home, settings> slot;
void show() { slot.visit([](lv::mounted<home>&) {}, [](auto) {}); }
