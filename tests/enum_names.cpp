// An LVGL enumerator is written by its name in quotes: `.align = "center"`.
// The name is the enumerator without what all of its enum share, in lower case.

#include <lv/lv.hpp>

#include "check.hpp"

#include <string_view>

namespace {

using namespace lv::units;
using namespace std::string_view_literals;

// The names, as the compiler sees them.
static_assert(lv::detail::shared_prefix(lv::detail::enum_names<lv_align_t>) == "LV_ALIGN_"sv.size());
static_assert(lv::detail::short_enum_name<lv_align_t>(lv::detail::enum_names<lv_align_t>[0]) == "default");
static_assert(lv::detail::enumerator_named<lv_align_t>("center") == LV_ALIGN_CENTER);
static_assert(lv::detail::enumerator_named<lv_align_t>("out_right_bottom") == LV_ALIGN_OUT_RIGHT_BOTTOM);
static_assert(lv::detail::enumerator_named<lv_flex_flow_t>("row_wrap") == LV_FLEX_FLOW_ROW_WRAP);
static_assert(lv::detail::enumerator_named<lv_flex_align_t>("space_between") == LV_FLEX_ALIGN_SPACE_BETWEEN);
// "OFF" and "ON" share a letter after the underscore: the names start after the underscore.
static_assert(lv::detail::shared_prefix(lv::detail::enum_names<lv_scrollbar_mode_t>) == "LV_SCROLLBAR_MODE_"sv.size());
static_assert(lv::detail::enumerator_named<lv_scrollbar_mode_t>("on") == LV_SCROLLBAR_MODE_ON);
static_assert(lv::detail::lowercase("Top_LEFT9") == "top_left9");
static_assert(lv::detail::no_such_enumerator<lv_flex_flow_t>("sideways").starts_with(
    "lv: flex_flow has no value 'sideways'. Its values: row, column, "));

constexpr auto look = lv::style({.bg_grad_dir = "ver", .text_align = "right"});

constexpr auto ui = lv::column({.flex_align = {"center", "end", "space_between"},
                                .size = {200 * px, 100 * px}})[
    lv::label({.align = "bottom_right", .id = "corner", .style = look, .text = "x"}),
    lv::label({.id = "plain", .text = "y"})];

void names_reach_lvgl() {
  auto screen = lv::screen_active();
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  lv_obj_t* column = lv_obj_get_child(screen.raw(), 0);
  check(lv_obj_get_style_flex_main_place(column, LV_PART_MAIN) == LV_FLEX_ALIGN_CENTER);
  check(lv_obj_get_style_flex_cross_place(column, LV_PART_MAIN) == LV_FLEX_ALIGN_END);
  check(lv_obj_get_style_flex_track_place(column, LV_PART_MAIN) == LV_FLEX_ALIGN_SPACE_BETWEEN);

  auto corner = root->get<"corner">();
  check(lv_obj_get_style_align(corner.raw(), LV_PART_MAIN) == LV_ALIGN_BOTTOM_RIGHT);
  check(lv_obj_get_style_bg_grad_dir(corner.raw(), LV_PART_MAIN) == LV_GRAD_DIR_VER);
  check(lv_obj_get_style_text_align(corner.raw(), LV_PART_MAIN) == LV_TEXT_ALIGN_RIGHT);

  // A view takes names too, and the C enumerator still works: it is the door.
  auto plain = root->get<"plain">();
  plain.set({.align = "top_mid"});
  check(lv_obj_get_style_align(plain.raw(), LV_PART_MAIN) == LV_ALIGN_TOP_MID);
  plain.set({.align = LV_ALIGN_LEFT_MID});
  check(lv_obj_get_style_align(plain.raw(), LV_PART_MAIN) == LV_ALIGN_LEFT_MID);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  names_reach_lvgl();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
