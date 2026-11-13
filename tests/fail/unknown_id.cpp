#include <lv/lv.hpp>
constexpr auto ui = lv::box()[lv::label({.id = "title"})];
void f(lv::obj screen) { lv::mount<ui>(screen)->get<"titel">(); }
