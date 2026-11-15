#include <lv/lv.hpp>
lv::task<> f(lv::obj box) { co_await lv::animate(box, {.width = {0, 100}}, 100 * lv::ms); }
