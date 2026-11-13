#include <lv/lv.hpp>
lv::task<int> f(lv::view<"button"> ok) {
  auto outcome = co_await lv::first(lv::next<LV_EVENT_CLICKED>(ok) | lv::as<"accepted">,
                                    lv::sleep(5 * lv::sec));
  co_return outcome.match<int>({.accepted = [] { return 1; }, .elapsd = [] { return 2; }, .gone = [] { return 3; }});
}
