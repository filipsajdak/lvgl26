#include <lv/lv.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
constexpr auto t = []<other P> { return lv::label({.text = "x"}); };
constexpr auto ui = lv::row[lv::for_each<subpage>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
