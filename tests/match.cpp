// lv::match is constexpr, so most of its tests are static assertions.
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

enum class mode { off, eco, sport };

constexpr const char* name_of(mode m) {
  return lv::match(m)(lv::is<mode::off> >> [] { return "Off"; },
                      lv::is<mode::eco> >> [] { return "Eco"; },
                      lv::is<mode::sport> >> [] { return "Sport"; });
}
static_assert(std::string_view(name_of(mode::off)) == "Off");
static_assert(std::string_view(name_of(mode::sport)) == "Sport");

// Several values in one arm, and a handler that takes the value.
constexpr int power(mode m) {
  return lv::match(m)(lv::is<mode::off> >> [] { return 0; },
                      lv::is<mode::eco, mode::sport> >> [](mode x) { return x == mode::eco ? 60 : 100; });
}
static_assert(power(mode::off) == 0 && power(mode::eco) == 60 && power(mode::sport) == 100);

// Other types need lv::otherwise; bool is covered by its two values.
constexpr int bucket(int n) {
  return lv::match(n)(lv::is<1, 2> >> [] { return 0; }, lv::is<3> >> [] { return 1; },
                      lv::otherwise >> [](int v) { return v; });
}
static_assert(bucket(1) == 0 && bucket(2) == 0 && bucket(3) == 1 && bucket(42) == 42);

constexpr char letter(bool on) {
  return lv::match(on)(lv::is<true> >> [] { return 'y'; }, lv::is<false> >> [] { return 'n'; });
}
static_assert(letter(true) == 'y' && letter(false) == 'n');

// The arms may return different types that have a common one, or nothing at all.
static_assert(std::is_same_v<decltype(lv::match(1)(lv::is<1> >> [] { return 1; },
                                                   lv::otherwise >> [] { return 2.5; })),
                             double>);

// LVGL's own C enums work the same way.
constexpr bool is_horizontal(lv_flex_flow_t flow) {
  return lv::match(flow)(lv::is<LV_FLEX_FLOW_ROW, LV_FLEX_FLOW_ROW_WRAP, LV_FLEX_FLOW_ROW_REVERSE,
                                LV_FLEX_FLOW_ROW_WRAP_REVERSE> >> [] { return true; },
                         lv::otherwise >> [] { return false; });
}
static_assert(is_horizontal(LV_FLEX_FLOW_ROW) && !is_horizontal(LV_FLEX_FLOW_COLUMN));

}  // namespace

int main() {
  // Arms that return nothing, with captures, at run time.
  int offs = 0, others = 0;
  for (mode m : {mode::off, mode::eco, mode::off, mode::sport})
    lv::match(m)(lv::is<mode::off> >> [&] { ++offs; }, lv::otherwise >> [&] { ++others; });
  check(offs == 2 && others == 2);
  check(std::strcmp(name_of(mode::eco), "Eco") == 0);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
