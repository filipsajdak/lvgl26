// A program whose settings come from tests/settings/project.json, through lvgl26_settings().
#include <lv/lv.hpp>

#include "check.hpp"

static_assert(!lv::detail::project<lv::override>.allow.threads);
static_assert(lv::detail::project<lv::override>.rules.raw_style_values);
static_assert(lv::detail::project<lv::override>.limits.compile_time_image.value == 64);

using namespace lv::units;

int settings_library_answer();

constexpr auto ui = lv::box({.id = "card", .style = {.radius = 8 * px}});  // rules.raw_style_values

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto root = lv::mount<ui>(lv::screen_active());
  check(root.has_value());
  check(settings_library_answer() == 42);
  check(lv_obj_get_style_radius(root->get<"card">().raw(), LV_PART_MAIN) == 8);
  root.unmount();
  lv_deinit();
  return failures;
}
