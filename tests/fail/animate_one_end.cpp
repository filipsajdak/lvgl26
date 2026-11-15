#include <lv/lv.hpp>
lv::task<> f(lv::view<"arc"> arc) { co_await lv::animate(arc, {.value = {100}}, 100 * lv::ms); }
