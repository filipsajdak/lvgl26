#include <lv/lv.hpp>
constexpr auto ui = lv::node<"obj">(lv::id<"a">, lv::node<"label">(lv::id<"a">));
void f(lv::obj screen) { lv::mount(ui, screen); }
