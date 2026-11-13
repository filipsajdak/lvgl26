#include <lv/lv.hpp>
constexpr auto ui = lv::node<"arc">(lv::p<"rnge">(0, 240));
void f(lv::obj screen) { lv::mount(ui, screen); }
