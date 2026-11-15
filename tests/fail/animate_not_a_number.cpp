#include <lv/lv.hpp>
lv::task<> f(lv::view<"label"> label) {
  co_await lv::animate(label, {.text = {0, 1}}, 100 * lv::ms);
}
