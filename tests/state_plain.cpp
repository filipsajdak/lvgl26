// The setting that allows plain integer states, for code that has no better name for a number.
#include <lv/lv.hpp>

#include "check.hpp"

template <>
inline constexpr lv::config lv::cfg<lv::override>{.raw_integer_state = true};

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  lv::state<int> count{3};
  auto bar = lv::screen_active().add<"bar">().set({.value = lv::bind(count)});
  check(bar.get<"value">() == 3);
  count = 9;
  check(bar.get<"value">() == 9);

  count.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
