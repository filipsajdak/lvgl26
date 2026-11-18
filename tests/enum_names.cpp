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
// LVGL marks an internal enumerator with a leading underscore, as in
// _LV_IMAGE_ALIGN_AUTO_TRANSFORM. The others are shortened all the same, and the
// odd one keeps its whole name.
static_assert(lv::detail::shared_prefix(lv::detail::enum_names<lv_image_align_t>) == "LV_IMAGE_ALIGN_"sv.size());
static_assert(lv::detail::enumerator_named<lv_image_align_t>("top_left") == LV_IMAGE_ALIGN_TOP_LEFT);
static_assert(lv::detail::enumerator_named<lv_image_align_t>("cover") == LV_IMAGE_ALIGN_COVER);
static_assert(lv::detail::enumerator_named<lv_image_align_t>("_lv_image_align_auto_transform") ==
              _LV_IMAGE_ALIGN_AUTO_TRANSFORM);
static_assert(lv::detail::no_such_enumerator<lv_image_align_t>("middle").starts_with(
    "lv: image_align has no value 'middle'. Its values: default, top_left, "));
// The odd one may come first, and an enum may have no LV_ names at all.
constexpr lv::detail::enum_name odd_first[] = {{"_LV_THING_MARK", 0}, {"LV_THING_ONE", 1}, {"LV_THING_TWO", 2}};
static_assert(lv::detail::shared_prefix(odd_first) == "LV_THING_"sv.size());
static_assert(lv::detail::short_name(odd_first[0].name, lv::detail::prefix_of(odd_first)) == "_lv_thing_mark");
static_assert(lv::detail::short_name(odd_first[2].name, lv::detail::prefix_of(odd_first)) == "two");
// Names that differ after the first underscore share only that much, whichever sorts first.
constexpr lv::detail::enum_name apart[] = {{"LV_A_B_C", 0}, {"LV_A_C_D", 1}};
static_assert(lv::detail::shared_prefix(apart) == "LV_A_"sv.size());
// An enum whose names share only "LV_" has no name of its own in the message.
enum two_families { LV_FOO_ONE, LV_BAR_TWO };
static_assert(lv::detail::no_such_enumerator<two_families>("x").starts_with("lv:  has no value 'x'. Its values: foo_one, bar_two"));
constexpr lv::detail::enum_name not_lvgl[] = {{"RED_ONE", 0}, {"RED_TWO", 1}};
static_assert(lv::detail::shared_prefix(not_lvgl) == "RED_"sv.size());
static_assert(lv::detail::short_name(not_lvgl[1].name, lv::detail::prefix_of(not_lvgl)) == "two");
static_assert(lv::detail::no_such_enumerator<lv_flex_flow_t>("sideways").starts_with(
    "lv: flex_flow has no value 'sideways'. Its values: row, column, "));

constexpr auto look = lv::style({.bg_grad_dir = "ver", .text_align = "right"});

constexpr auto ui = lv::column({.flex_align = {"center", "end", "space_between"},
                                .size = {200 * px, 100 * px}})[
    lv::label({.align = "bottom_right", .id = "corner", .style = look, .text = "x"}),
    lv::label({.id = "plain", .text = "y"}),
    lv::image({.id = "picture", .inner_align = "top_left"}),
    // A name beside a value of another kind: lv_roller_set_options takes a text, then a mode.
    lv::roller({.id = "days", .options = {"Mon\nTue\nWed", "infinite"}})];

void names_reach_lvgl() {
  auto screen = lv::screen_active();
  auto root = lv::mount<ui>(screen);
  check(static_cast<bool>(root));
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

  auto picture = root->get<"picture">();
  check(lv_image_get_inner_align(picture.raw()) == LV_IMAGE_ALIGN_TOP_LEFT);
  picture.set({.inner_align = "cover"});
  check(lv_image_get_inner_align(picture.raw()) == LV_IMAGE_ALIGN_COVER);
}

// The other values beside a name need not be constants: only the name is read at
// compile time.
void names_beside_values() {
  auto screen = lv::screen_active();
  auto root = lv::mount<ui>(screen);
  check(static_cast<bool>(root));
  auto days = root->get<"days">();
  check(lv_roller_get_option_count(days.raw()) == 3);
  // An infinite roller repeats its options; a normal one does not.
  check(std::string_view{lv_roller_get_options(days.raw())}.starts_with("Mon\nTue\nWed\nMon"));

  char months[] = "Jan\nFeb";
  const char* given = months;
  days.set({.options = {given, "normal"}});
  check(lv_roller_get_option_count(days.raw()) == 2);
  check(std::string_view{lv_roller_get_options(days.raw())} == "Jan\nFeb");

  // A name, a C enumerator and a value made at run time may be mixed.
  auto column = lv::view<"obj">{lv_obj_get_child(screen.raw(), 0)};
  const lv_flex_align_t cross = days.raw() != nullptr ? LV_FLEX_ALIGN_START : LV_FLEX_ALIGN_END;
  column.set({.flex_align = {"space_around", cross, "center"}});
  check(lv_obj_get_style_flex_main_place(column.raw(), LV_PART_MAIN) == LV_FLEX_ALIGN_SPACE_AROUND);
  check(lv_obj_get_style_flex_cross_place(column.raw(), LV_PART_MAIN) == LV_FLEX_ALIGN_START);
  check(lv_obj_get_style_flex_track_place(column.raw(), LV_PART_MAIN) == LV_FLEX_ALIGN_CENTER);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  names_reach_lvgl();
  lv_obj_clean(lv::screen_active().raw());
  names_beside_values();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
