#include <lv/lv.hpp>
struct offset { int value; };
constinit lv::state<offset> w{offset{10}};
constexpr auto ui = lv::box({.pos = lv::bind(w)});
void f(lv::obj s) { lv::mount<ui>(s); }
