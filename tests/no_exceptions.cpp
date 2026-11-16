// The library is usable in a program built with -fno-exceptions and -fno-rtti, as
// microcontroller projects are: this file is compiled with both (see CMakeLists.txt).
// It uses each part that has compile-time checks, since those are what used `throw`.
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

using namespace lv::units;

constinit lv::state ticks{lv::counter{}};
constinit lv::computed some{[](lv::counter c) { return c.value > 0; }, ticks};

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "count", .text = lv::bind(ticks, "{} ticks")}),
    lv::arc({.id = "dial", .range = {0, 240}, .value = 120}),
    lv::label({.hidden = lv::bind(some), .id = "note", .text = "none yet"})];

lv::task<> run(lv::view<"arc"> dial) {
  co_await lv::sleep(50 * ms);
  ++ticks;
  dial.set<"value">(60);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  {
    auto root = lv::mount<ui>(lv::screen_active());
    check(root.has_value());
    if (root) {
      check(lv::spawn(run(root->get<"dial">())));
      lv_test_wait(100);
      check(ticks.get().value == 1 && some.get());
      check(root->get<"dial">().get<"value">() == 60);
      check(std::strcmp(root->get<"count">().get<"text">(), "1 ticks") == 0);
    }
  }
  some.reset();
  ticks.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
