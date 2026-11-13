#include <lv/lv.hpp>
constexpr auto ui = lv::node<"obj">(lv::node<"label">(lv::id<"title">));
void f(lv::obj screen) { lv::mount(ui, screen)->get<"titel">(); }
