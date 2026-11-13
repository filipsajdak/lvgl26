#include <lv/lv.hpp>
constexpr auto ui = lv::box({.id = "a"})[lv::label({.id = "a"})];
void f(lv::obj screen) { lv::mount<ui>(screen); }
