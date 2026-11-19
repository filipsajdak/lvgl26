#include <lv/lv.hpp>
constexpr auto home = lv::column[lv::label({.text = "Home"})];
constexpr auto settings = lv::column[lv::label({.text = "Settings"})];
constexpr auto other = lv::column[lv::label({.text = "Other"})];
constinit lv::one_of<home, settings> slot;
void show() { slot.visit([](lv::held<home>) { return 1; }, [](auto) { return "two"; }); }
