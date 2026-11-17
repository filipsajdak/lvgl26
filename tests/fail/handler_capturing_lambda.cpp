#include <lv/lv.hpp>
// A handler in a constexpr tree is a function pointer: a lambda with a capture has no state to keep.
int taps = 0;
constexpr auto make(int n) { return lv::button({.on_clicked = [n] { taps += n; }}); }
constexpr auto ui = make(1);
