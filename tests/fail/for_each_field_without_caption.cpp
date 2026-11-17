#include <lv/lv.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
struct plain { int a; };
constexpr auto t = []<lv::field F> { return lv::label({.text = F.caption}); };
constexpr auto ui = lv::row[lv::for_each_field<plain>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
