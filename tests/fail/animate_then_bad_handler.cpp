#include <lv/lv.hpp>
lv::task<> f(lv::view<"arc"> arc) {
  co_await lv::first(lv::animate(arc, {.value = {0, 100}}, 100 * lv::ms).then([](int*) {}));
}
