// lv::detail::add_style: every style lvgl26 adds goes through it, and a full heap fails the add
// instead of LVGL writing through a null pointer.

#include <lv/lv.hpp>

#include "check.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace {

// Stand-ins for an lv_obj_add_style that reports its failure, as LVGL's does from
// lvgl/lvgl#11015 on.
int added = 0;
lv_result_t add_that_fails(lv_obj_t*, const lv_style_t*, lv_style_selector_t) { return LV_RESULT_INVALID; }
lv_result_t add_that_works(lv_obj_t* o, const lv_style_t* style, lv_style_selector_t selector) {
  lv_obj_add_style(o, style, selector);  // C API
  ++added;
  return LV_RESULT_OK;
}

static_assert(lv::detail::reports_failure<&add_that_fails> && lv::detail::reports_failure<&add_that_works>);

// Constant styles with no property, one more than LVGL keeps on a widget, and one that sets a
// property, to see whether LVGL took it.
constexpr std::size_t many = lv::detail::most_styles_of_a_widget + 1;
constexpr lv_style_const_prop_t no_props[] = {{.prop = LV_STYLE_PROP_INV, .value = {.num = 0}}};  // C API
const std::array<lv_style_t, many> empty_styles = [] {
  std::array<lv_style_t, many> out{};
  for (auto& s : out) s = lv::detail::const_style(no_props);
  return out;
}();
constexpr lv_opa_t marked_opa = 42;
constexpr lv_style_const_prop_t marked_props[] = {{.prop = LV_STYLE_BG_OPA, .value = {.num = marked_opa}},  // C API
                                                  {.prop = LV_STYLE_PROP_INV, .value = {.num = 0}}};
const lv_style_t marked = lv::detail::const_style(marked_props);

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// Where LVGL reports the failure, its result decides.
void a_reported_result_decides() {
  lv_obj_t* o = lv_obj_create(lv_screen_active());
  check(!lv::detail::add_style_with<&add_that_fails>(o, &empty_styles[0], LV_PART_MAIN));
  check(lv::detail::add_style_with<&add_that_works>(o, &empty_styles[0], LV_PART_MAIN) && added == 1);
  lv_obj_delete(o);
}

// Where it does not (both pins), the add is refused when the longest list of styles would not fit.
void a_full_heap_refuses_the_add() {
  lv_obj_t* o = lv_obj_create(lv_screen_active());
  const std::size_t before = used_bytes();
  std::vector<void*> taken;
  for (void* p = lv_malloc(lv::detail::style_list_bound); p != nullptr; p = lv_malloc(lv::detail::style_list_bound))
    taken.push_back(p);
  for (void* p = lv_malloc(64); p != nullptr; p = lv_malloc(64)) taken.push_back(p);
  check(!lv::detail::room_for_a_style());
  check(!lv::detail::add_style(o, &empty_styles[0], LV_PART_MAIN));
  for (void* p : taken) lv_free(p);
  check(used_bytes() == before);  // the refused add took nothing
  check(lv::detail::room_for_a_style());
  check(lv::detail::add_style(o, &empty_styles[0], LV_PART_MAIN));
  lv_obj_delete(o);
}

constexpr std::size_t percent = 100;
constexpr std::size_t bound_slack_percent = 5;  // the measured growth is 984 of 1008 bytes: 2.4 percent under

// What the bound relies on, measured: a full list of styles grows the heap by no more than the
// probe, and LVGL refuses one style more than most_styles_of_a_widget.
void the_bound_holds() {
  lv_obj_t* o = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(o);
  lv_obj_add_style(o, &empty_styles[0], LV_PART_MAIN);  // C API: the list exists from here on
  const std::size_t one = used_bytes();
  for (std::size_t i = 1; i < lv::detail::most_styles_of_a_widget; ++i)
    lv_obj_add_style(o, &empty_styles[i], LV_PART_MAIN);  // C API
  const std::size_t grown = used_bytes() - one;
  std::printf("a full list of styles grew the heap by %zu bytes; the probe is %zu\n", grown,
              lv::detail::style_list_bound);
  check(grown <= lv::detail::style_list_bound);
  // And the bound is close to what a full list takes: a bound of three pointers an entry would pass the check above.
  check(lv::detail::style_list_bound * percent <= grown * (percent + bound_slack_percent));
  check(grown >= (lv::detail::most_styles_of_a_widget - 1) * sizeof(void*));  // an entry holds a pointer
  lv_obj_add_style(o, &marked, LV_PART_MAIN);  // C API: one more than LVGL keeps
  check(lv_obj_get_style_bg_opa(o, LV_PART_MAIN) != marked_opa);
  lv_obj_delete(o);
}

// One style fewer, and the same style is taken: the cap is exactly most_styles_of_a_widget.
void the_last_style_is_taken() {
  lv_obj_t* o = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(o);
  for (std::size_t i = 1; i < lv::detail::most_styles_of_a_widget; ++i)
    lv_obj_add_style(o, &empty_styles[i], LV_PART_MAIN);  // C API
  lv_obj_add_style(o, &marked, LV_PART_MAIN);  // C API: the 63rd
  check(lv_obj_get_style_bg_opa(o, LV_PART_MAIN) == marked_opa);
  lv_obj_delete(o);
}

}  // namespace

int main() {
  lv::application app;
  constinit static lv::frame<64, 48> frame;
  lv::display display{app, frame};
  a_reported_result_decides();
  a_full_heap_refuses_the_add();
  the_bound_holds();
  the_last_style_is_taken();
  return failures;
}
