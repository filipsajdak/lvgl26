#include <lv/lv.hpp>
lv::task<> f(lv::view<"button"> b) {
  co_await lv::first(b.next().clicked >> [] {}, lv::sleep(5 * lv::sec));
}
