#include <lv/lv.hpp>
#include <lv/one_of.hpp>
constexpr auto home = lv::column[lv::label({.text = "Home"})];
constexpr auto settings = lv::column[lv::label({.text = "Settings"})];
constinit lv::one_of<home, settings> slot;
#include <string_view>
void show() { slot.visit<int>([](lv::held<home>) { return 1; }, [](auto) { return std::string_view{"two"}; }); }
