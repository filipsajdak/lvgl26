#include <lv/lv.hpp>
struct px { int value; };
constinit lv::state<px> w{px{10}};
constexpr auto ui = lv::box({.pos = lv::bind(w)});
void f(lv::obj s) { lv::mount<ui>(s); }
