#include <lv/lv.hpp>
constinit lv::state<int> w{10};
constexpr auto ui = lv::box({.pos = lv::bind(w)});
void f(lv::obj s) { lv::mount<ui>(s); }
