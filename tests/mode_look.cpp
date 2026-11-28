// lv::mode_look: a look whose styles follow the mode of a display that follows a state. A node, a group and a
// row of lv::each take it; a switch puts the styles of the other mode in place; what the node's own style says
// still wins; a widget made later takes the mode of the moment.
//
//   mode_look <day.png> <dark.png>

#include <lv/lv.hpp>
#include <lv/list.hpp>
#include <lv/mode_look.hpp>
#include <lv/theme.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {

using namespace lv::units;

constexpr char day_json[] = {
#embed "mode_day.json"
    , 0};
constexpr char dark_json[] = {
#embed "mode_dark.json"
    , 0};
constexpr auto day_tokens = lv::tokens<day_json>;
constexpr auto dark_tokens = lv::tokens<dark_json>;

struct day_design {};
struct dark_design {};

enum class mode { day, dark };
constinit lv::state<mode> current{mode::day};

}  // namespace

//! [designs]
// One token file for each mode, of one shape, named in the order of the enumerators of the mode.
template <>
inline constexpr lv::design_of<decltype(day_tokens)> lv::design<day_design>{.tokens = day_tokens};
template <>
inline constexpr lv::design_of<decltype(dark_tokens)> lv::design<dark_design>{.tokens = dark_tokens};
template <>
inline constexpr lv::mode_designs_of<day_design, dark_design> lv::mode_designs<lv::override>{};
//! [designs]

namespace {

constexpr lv_color_t rgb(std::uint32_t h) { return {.blue = static_cast<std::uint8_t>(h), .green = static_cast<std::uint8_t>(h >> 8), .red = static_cast<std::uint8_t>(h >> 16)}; }

constexpr lv_color_t day_card = rgb(0xffffff);
constexpr lv_color_t dark_card = rgb(0x2a221d);
constexpr lv_color_t day_page = rgb(0xf4efe8);
constexpr lv_color_t dark_page = rgb(0x161210);
constexpr lv_color_t day_accent = rgb(0xd98a3d);
constexpr lv_color_t dark_accent = rgb(0xe0a050);

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

struct item_id {
  int value;
};
struct item {
  item_id id;
};
constinit lv::list<item, 4> items{};
constinit lv::frame<240, 200> frame;

//! [tree]
constexpr auto row = lv::box({.size = {120 * px, 24 * px}, .look = lv::mode_look<"tile">});

// A group (which starts without the theme's styles), a node with a mode look and an own style that wins, a plain
// node with no mode look, and a list of rows.
constexpr auto ui = lv::column(
    {.size = {240 * px, 200 * px}, .look = lv::mode_look<"page">},
    lv::group({.id = "group", .size = {240 * px, 40 * px}, .look = lv::mode_look<"tile">}),
    lv::box({.id = "tile", .size = {120 * px, 40 * px}, .look = lv::mode_look<"tile">}),
    lv::box({.id = "own", .size = {120 * px, 40 * px}, .look = lv::mode_look<"tile">, .style = {.bg_color = lv::raw(rgb(0x00ff00))}}),
    lv::box({.id = "plain", .size = {120 * px, 40 * px}, .style = lv::look<"tile", day_design>}),
    lv::each(items, &item::id, row));
//! [tree]

lv_color_t bg_of(lv_obj_t* o) { return lv_obj_get_style_bg_color(o, LV_PART_MAIN); }

lv_color_t bg_pressed(lv_obj_t* o) {
  lv_obj_add_state(o, LV_STATE_PRESSED);
  const lv_color_t c = lv_obj_get_style_bg_color(o, LV_PART_MAIN);
  lv_obj_remove_state(o, LV_STATE_PRESSED);
  return c;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::printf("usage: mode_look <day.png> <dark.png>\n");
    return 2;
  }
  lv::application app;
  lv::display display{app, frame};
  //! [follow]
  // The display follows the state; a theme per value (here none: only the mode looks follow).
  display.follow(current, {lv::theme_styles{}, lv::theme_styles{}});
  //! [follow]

  check(items.push_back({{1}}).has_value());
  check(items.push_back({{2}}).has_value());
  auto w = lv::mount<ui>(display.screen());
  check(w.has_value());
  if (!w) return 1;
  lv_obj_t* root = w->root().raw();
  lv_obj_t* group = w->get<"group">().raw();
  lv_obj_t* tile = w->get<"tile">().raw();
  lv_obj_t* own = w->get<"own">().raw();
  lv_obj_t* plain = w->get<"plain">().raw();
  lv_obj_t* a_row = lv_obj_get_child(root, 4);
  check(a_row != nullptr);

  check(same(bg_of(root), day_page) && same(bg_of(group), day_card) && same(bg_of(tile), day_card) && same(bg_of(a_row), day_card));
  check(same(bg_pressed(tile), day_accent));  // a layer: a nested group is a state
  check(lv_test_screenshot_compare(argv[1]) == LV_TEST_SCREENSHOT_RESULT_PASSED);

  //! [switch]
  current = mode::dark;  // every mode look is restyled in place
  //! [switch]
  check(same(bg_of(root), dark_page) && same(bg_of(group), dark_card) && same(bg_of(tile), dark_card) && same(bg_of(a_row), dark_card));
  check(same(bg_pressed(tile), dark_accent));
  check(same(bg_of(own), rgb(0x00ff00)));  // the node's own style still wins
  check(same(bg_of(plain), day_card));              // a plain look is constant: it never follows
  check(lv_test_screenshot_compare(argv[2]) == LV_TEST_SCREENSHOT_RESULT_PASSED);

  // A row made after the switch takes the mode of the moment, and the next switch moves it too.
  check(items.push_back({{3}}).has_value());
  lv::settle();
  lv_obj_t* late_row = lv_obj_get_child(root, 6);
  check(late_row != nullptr && same(bg_of(late_row), dark_card));
  current = mode::day;
  check(same(bg_of(late_row), day_card) && same(bg_of(tile), day_card) && same(bg_of(root), day_page));

  // A switch allocates nothing (after one pair, which LVGL's own caches take).
  current = mode::dark;
  current = mode::day;
  lv_mem_monitor_t before, after;
  lv_mem_monitor(&before);
  current = mode::dark;
  current = mode::day;
  lv_mem_monitor(&after);
  check(after.free_size == before.free_size);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
