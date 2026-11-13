#include <lv/lv.hpp>

#include "check.hpp"

#include <type_traits>

// This file turns the token rule off, to test styles on their own and the setting itself.
template <>
inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};

namespace {

using namespace lv::units;

// A named style, reused by two nodes.
constexpr auto card = lv::style(
    {.bg_color = lv::rgb(0x1b2430), .bg_opa = 100 * pct, .border_width = 0 * px, .pad_all = 12 * px,
     .radius = 8 * px});

constexpr auto ui = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320 * px, 240 * px},
     .style = {.bg_color = lv::rgb(0x0d1117), .bg_opa = 100 * pct, .radius = 0 * px}})[
    lv::column({.id = "card", .size = {200 * px, lv::size_content}, .style = card})[
        lv::label({.id = "title", .style = {.text_color = lv::rgb(0xe6edf3)}, .text = "Speed"}),
        lv::label({.id = "unit", .style = {.text_color = lv::rgb(0x8b949e), .text_opa = 80 * pct},
                   .text = "km/h"})],
    lv::box({.id = "plain", .size = {40 * px, 10 * px}})];

// The types of a few style members.
using style_type = std::remove_cvref_t<decltype(card)>;
static_assert(std::is_same_v<decltype(style_type{}.radius), lv::arg<lv::styled<lv::length>>>);
static_assert(std::is_same_v<decltype(style_type{}.bg_opa), lv::arg<lv::styled<lv::opa>>>);
static_assert(std::is_same_v<decltype(style_type{}.bg_color), lv::arg<lv::styled<lv_color_t>>>);
// Enums are structure, not design: they stay plain.
static_assert(std::is_same_v<decltype(style_type{}.text_align), lv::arg<lv_text_align_t>>);
static_assert(std::is_same_v<decltype(style_type{}.transform_rotation),
                             lv::arg<lv::styled<lv::strict<std::int32_t>>>>);

// card sets five properties; pad_all has no LV_STYLE_* id, so four go to the const style.
static_assert(lv::detail::const_count<card> == 4);
static_assert(lv::detail::const_props<card>().back().prop == LV_STYLE_PROP_INV);

bool same(lv_color_t a, lv_color_t b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

void applies(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  if (!root) return;

  auto c = root->get<"card">();
  check(same(c.get<"style_bg_color">(LV_PART_MAIN), lv::rgb(0x1b2430)));
  check(c.get<"style_radius">(LV_PART_MAIN) == 8);
  check(c.get<"style_border_width">(LV_PART_MAIN) == 0);
  check(c.get<"style_pad_top">(LV_PART_MAIN) == 12);  // from pad_all, a local style
  check(c.get<"style_bg_opa">(LV_PART_MAIN) == LV_OPA_COVER);

  check(same(root->get<"title">().get<"style_text_color">(LV_PART_MAIN), lv::rgb(0xe6edf3)));
  check(root->get<"unit">().get<"style_text_opa">(LV_PART_MAIN) == 204);

  // Changing a style at run time uses local styles and wins over the const one.
  c.set({.style = {.radius = 2 * px}});
  check(c.get<"style_radius">(LV_PART_MAIN) == 2);
}

// A node whose style is all const must not cost more heap than one with no style,
// apart from LVGL's entry that points to the style.
void const_styles_use_no_heap_for_values(lv::obj screen) {
  static constexpr auto bare = lv::box();
  static constexpr auto styled = lv::box(
      {.style = {.bg_color = lv::rgb(0x112233), .bg_opa = 100 * pct, .border_color = lv::rgb(0x445566),
                 .border_width = 2 * px, .outline_width = 1 * px, .radius = 4 * px,
                 .shadow_width = 3 * px, .text_color = lv::rgb(0x778899)}});
  static constexpr auto local = lv::box({.style = {.pad_all = 4 * px}});

  auto cost = [&](auto mount) {
    const auto before = free_bytes();
    auto m = mount();
    return before - free_bytes();
  };
  const auto bare_cost = cost([&] { return lv::mount<bare>(screen); });
  const auto styled_cost = cost([&] { return lv::mount<styled>(screen); });
  const auto local_cost = cost([&] { return lv::mount<local>(screen); });

  // Eight const properties cost one small style entry; four local ones cost more.
  check(styled_cost - bare_cost <= 32);
  check(local_cost > styled_cost);
}

void renders(const char* reference) {
  auto root = lv::mount<ui>(lv::screen_active());
  check(lv_test_screenshot_compare(reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: styles <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  applies(screen);
  const_styles_use_no_heap_for_values(screen);
  renders(argv[1]);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
