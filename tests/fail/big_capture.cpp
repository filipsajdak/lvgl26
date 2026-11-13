#include <lv/lv.hpp>
void f(lv::view<"button"> b) {
  int x = 0, y = 0;
  b.on<LV_EVENT_CLICKED>([&x, &y] { ++x, ++y; });
}
