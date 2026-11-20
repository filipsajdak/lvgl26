#include <lv/lv.hpp>
#include <lv/repeat.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
constexpr auto t = []<subpage P> { return lv::label({.id = "tile", .text = "x"}); };
constexpr auto ui = lv::row[lv::for_each<subpage>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
