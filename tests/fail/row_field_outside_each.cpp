#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track {
  const char* title;
};
constexpr auto ui = lv::column[lv::label({.text = lv::bind(&track::title)})];
void f(lv::obj screen) { lv::mount<ui>(screen); }
