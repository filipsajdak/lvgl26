#include <lv/lv.hpp>
// The part of lv::for_each runs in the compiler: a capture is refused at the call.
enum class subpage { none, print };
constexpr auto ui = lv::row[lv::for_each<subpage>([k = 1]<subpage P> { return lv::label({.text = "x"}); })];
void f(lv::obj s) { lv::mount<ui>(s); }
