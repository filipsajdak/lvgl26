// A setter that takes an unsigned number, such as lv_spinner_set_arc_sweep(obj,
// uint32_t angle), takes a unit value that is not negative. LVGL receives it unsigned.
#include <lv/lv.hpp>
#include <lv/runtime.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstring>
#include <limits>

namespace {

using namespace lv::units;

static_assert(std::is_same_v<decltype(lv::props<"spinner">{}.arc_sweep),
                             lv::arg<lv::nonnegative<lv::angle>>>);
static_assert(std::is_same_v<decltype(lv::props<"scale">{}.angle_range),
                             lv::arg<lv::nonnegative<lv::angle>>>);
// A signed setter keeps its plain unit.
static_assert(std::is_same_v<decltype(lv::props<"arc">{}.rotation), lv::arg<lv::angle>>);
static_assert(lv::detail::how_to_write(^^lv::nonnegative<lv::angle>) ==
              "an angle in whole degrees (90 * lv::deg), not negative");

// Both bounds pass: no angle, and the largest a signed 32-bit number holds.
constexpr std::int32_t widest = std::numeric_limits<std::int32_t>::max();
constexpr lv::nonnegative<lv::angle> none = 0 * deg;
constexpr lv::nonnegative<lv::angle> most = widest * deg;
static_assert(none.raw() == 0U && most.raw() == 2'147'483'647U);
static_assert(most.get() == widest * deg);
// It can be animated, as the angle it holds.
static_assert(requires { lv::animatable<"spinner">{}.arc_sweep; });
static_assert(lv::detail::can_run_between(none, most));

constexpr auto ui = lv::column[lv::spinner({.id = "spin", .arc_sweep = 270 * deg}),
                               lv::spinner({.id = "none", .arc_sweep = 0 * deg}),
                               lv::spinner({.id = "widest", .arc_sweep = widest * deg})];

void sweeps_reach_lvgl(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  check(lv_spinner_get_arc_sweep(root->get<"spin">().raw()) == 270);
  check(lv_spinner_get_arc_sweep(root->get<"none">().raw()) == 0);
  check(lv_spinner_get_arc_sweep(root->get<"widest">().raw()) == 2'147'483'647U);

  // A value made at run time is checked at run time.
  volatile std::int32_t degrees = 90;
  auto spin = root->get<"spin">();
  spin.set({.arc_sweep = degrees * deg});
  check(lv_spinner_get_arc_sweep(spin.raw()) == 90);

  // A state that the sweep follows.
  lv::state sweep{180 * deg};
  spin.set({.arc_sweep = lv::bind(sweep)});
  check(lv_spinner_get_arc_sweep(spin.raw()) == 180);
  sweep = 45 * deg;
  check(lv_spinner_get_arc_sweep(spin.raw()) == 45);
  lv_obj_clean(screen.raw());
}

// A document gives the sweep as a number, and a negative one is refused.
void sweeps_from_a_document(lv::obj screen) {
  using doc = lv::runtime<"obj", "spinner">;
  auto loaded = doc::load(R"({"type": "spinner", "id": "s", "props": {"arc_sweep": 120}})", screen);
  check(loaded.has_value());
  if (loaded) check(lv_spinner_get_arc_sweep(loaded->find("s").raw()) == 120);
  // No angle is the lower bound, and it fits.
  auto zero = doc::load(R"({"type": "spinner", "id": "z", "props": {"arc_sweep": 0}})", screen);
  check(zero.has_value());
  if (zero) check(lv_spinner_get_arc_sweep(zero->find("z").raw()) == 0);
  const auto refused = doc::load(R"({"type": "spinner", "props": {"arc_sweep": -10}})", screen);
  check(!refused.has_value());
  if (!refused) check(refused.error().error == lv::load_error::bad_value && refused.error().at == "-10");
  lv_obj_clean(screen.raw());
}

// A negative value made at run time is a contract violation.
int violates(const char* which) {
  volatile std::int32_t below = -1;
  if (std::strcmp(which, "negative") == 0) {
    const lv::nonnegative<lv::angle> a = below * deg;
    return static_cast<int>(a.raw());
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1]));
    return 0;
  }
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();
  sweeps_reach_lvgl(screen);
  sweeps_from_a_document(screen);
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
