#include <lv/lv.hpp>
lv::task<int> f(lv::view<"button"> a, lv::view<"button"> b) {
  auto outcome = co_await lv::first(lv::next<LV_EVENT_CLICKED>(a), lv::next<LV_EVENT_PRESSED>(b) | lv::as<"clicked">);
  co_return outcome.match<int>({.clicked = [] { return 1; }, .gone = [] { return 2; }});
}
