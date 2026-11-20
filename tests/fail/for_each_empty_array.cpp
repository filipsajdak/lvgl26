#include <lv/lv.hpp>
#include <lv/repeat.hpp>
#include <array>
enum class subpage { none, print, scan };
enum class other { x };
struct level { int percent; };
constexpr std::array<level, 0> none{};
constexpr auto t = []<level L> { return lv::label({.text = "x"}); };
constexpr auto ui = lv::row[lv::for_each<none>(t)];
void f(lv::obj s) { lv::mount<ui>(s); }
