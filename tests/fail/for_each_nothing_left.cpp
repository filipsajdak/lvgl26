#include <lv/lv.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
constexpr auto t = []<subpage P> { return lv::label({.text = "x"}); };
constexpr auto ui = lv::row[lv::for_each<subpage, lv::but<subpage::none, subpage::print, subpage::scan>>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
