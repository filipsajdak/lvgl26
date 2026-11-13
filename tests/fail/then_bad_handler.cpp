#include <lv/lv.hpp>
lv::task<> f() { co_await lv::first(lv::sleep(1 * lv::sec).then([](int) {})); }
