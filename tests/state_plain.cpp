// The setting that allows plain integer states, for code that has no better name for a number.
#include <lv/list.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

template <>
inline constexpr lv::config lv::cfg<lv::override>{.rules = {.raw_integer_state = true}};

// With the setting, a plain integer names a row too; a number with a fraction still does not.
static_assert(lv::detail::row_key<int> && !lv::detail::row_key<double>);

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  lv::state<int> count{3};
  auto bar = lv::screen_active().add<"bar">().set({.value = lv::bind(count)});
  check(bar.get<"value">() == 3);
  count = 9;
  check(bar.get<"value">() == 9);

  // The setting also allows a computed whose result is a plain integer.
  lv::computed twice{[](int n) { return n * 2; }, count};
  auto other = lv::screen_active().add<"bar">().set({.value = lv::bind(twice)});
  check(twice.get() == 18 && other.get<"value">() == 18);
  count = 21;
  check(other.get<"value">() == 42);

  twice.reset();
  count.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
