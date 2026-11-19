// The start of lv::group, lv::row and lv::column: one constant style at the bottom of the cascade.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstddef>

namespace {

using namespace lv::units;

lv::application* the_app = nullptr;

constinit lv::frame<320, 240> frame;
constinit lv::frame<64, 48> small_frame;

// LVGL keeps one entry in a widget's list of styles for each style it adds.
constexpr std::size_t style_entry = 16;

std::int32_t layout_of(lv_obj_t* o) { return lv_obj_get_style_prop(o, LV_PART_MAIN, LV_STYLE_LAYOUT).num; }

constexpr auto group_ui = lv::group({.id = "g"});
constexpr auto row_ui = lv::row({.id = "r"});
constexpr auto column_ui = lv::column({.id = "c"});
constexpr auto own_width_ui = lv::group({.id = "g", .width = 30 * px});
constexpr auto own_flow_ui = lv::row({.flex_flow = "column", .id = "r"});
constexpr auto plain_ui = lv::box({});

// The start of each kind, as the cascade sees it.
void each_kind_starts_with_its_style() {
  auto g = lv::mount<group_ui>(lv::screen_active());
  auto r = lv::mount<row_ui>(lv::screen_active());
  auto c = lv::mount<column_ui>(lv::screen_active());
  check(g.has_value() && r.has_value() && c.has_value());
  if (!g || !r || !c) return;
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(g->root().raw()) == lv_obj_get_width(lv_screen_active()));
  check(lv_obj_get_height(g->root().raw()) == lv_obj_get_height(lv_screen_active()));
  check(lv_obj_get_style_flex_flow(r->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
  check(lv_obj_get_style_flex_flow(c->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  check(layout_of(r->root().raw()) == LV_LAYOUT_FLEX && layout_of(c->root().raw()) == LV_LAYOUT_FLEX);
}

// The node's own property beats the start.
void the_nodes_own_property_beats_the_start() {
  auto g = lv::mount<own_width_ui>(lv::screen_active());
  auto r = lv::mount<own_flow_ui>(lv::screen_active());
  check(g.has_value() && r.has_value());
  if (!g || !r) return;
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(g->root().raw()) == 30);
  check(lv_obj_get_height(g->root().raw()) == lv_obj_get_height(lv_screen_active()));
  check(lv_obj_get_style_flex_flow(r->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
}

// A style added after the widget is made beats the start: a local default could never yield to it.
void a_style_added_later_beats_the_start() {
  static constexpr lv_style_const_prop_t narrow_props[] = {{.prop = LV_STYLE_WIDTH, .value = {.num = 40}},  // C API
                                                           {.prop = LV_STYLE_PROP_INV, .value = {.num = 0}}};
  static const lv_style_t narrow = lv::detail::const_style(narrow_props);
  auto g = lv::mount<group_ui>(lv::screen_active());
  check(g.has_value());
  if (!g) return;
  lv_obj_add_style(g->root().raw(), &narrow, LV_PART_MAIN);
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(g->root().raw()) == 40);
}

// A theme is applied when the widget is made, before the start: the start beats it, as a local
// default did. A theme cannot turn a row into a column.
constexpr auto flow_theme = lv::theme({.obj = {.style = {.flex_flow = "column"}}});

void the_start_beats_a_theme() {
  lv::display display{*the_app, small_frame, flow_theme};
  auto r = lv::mount<row_ui>(display.screen());
  check(r.has_value());
  if (!r) return;
  check(lv_obj_get_style_flex_flow(r->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
}

// Where the start lives. It is no local style, as LVGL's own theme and class defaults are none:
// lv_obj_get_local_style_prop does not find it, the cascade does.
void the_start_is_no_local_style() {
  auto g = lv::mount<group_ui>(lv::screen_active());
  check(g.has_value());
  if (!g) return;
  lv_style_value_t v{};
  check(lv_obj_get_local_style_prop(g->root().raw(), LV_STYLE_WIDTH, &v, LV_PART_MAIN) != LV_STYLE_RES_FOUND);
  check(lv_obj_get_style_width(g->root().raw(), LV_PART_MAIN) == lv_pct(100));
}

// Removing the theme's styles keeps the start; removing every style removes it, as it removes a
// local style and the theme's: the row then stops laying its children out, as it did before.
void what_removes_the_start() {
  auto r = lv::mount<row_ui>(lv::screen_active());
  check(r.has_value());
  if (!r) return;
  lv_obj_t* o = r->root().raw();
  lv_obj_remove_theme(o, static_cast<lv_style_selector_t>(LV_PART_ANY) | LV_STATE_ANY);
  check(layout_of(o) == LV_LAYOUT_FLEX && lv_obj_get_style_flex_flow(o, LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
  lv_obj_remove_style_all(o);
  check(layout_of(o) == LV_LAYOUT_NONE);
}

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

std::size_t heap_of_mounted(auto mounted) {
  const auto start = used_bytes();
  auto built = mounted();
  return used_bytes() - start;
}

// The start is a constant style: a row costs one entry in LVGL's list of styles beyond a box, and
// a group, which drops the theme's styles, no more than a box.
void the_start_costs_one_style_entry() {
  heap_of_mounted([] { return lv::mount<plain_ui>(lv::screen_active()); });  // LVGL's first-use state
  const auto box = heap_of_mounted([] { return lv::mount<plain_ui>(lv::screen_active()); });
  const auto group = heap_of_mounted([] { return lv::mount<group_ui>(lv::screen_active()); });
  const auto row = heap_of_mounted([] { return lv::mount<row_ui>(lv::screen_active()); });
  std::printf("heap: a box %zu bytes, a group %zu, a row %zu\n", box, group, row);
  check(group <= box);
  check(row <= box + style_entry);
}

}  // namespace

int main() {
  lv::application app;
  the_app = &app;
  lv::display display{app, frame};
  the_start_costs_one_style_entry();  // first: other tests move the heap's free blocks
  each_kind_starts_with_its_style();
  the_nodes_own_property_beats_the_start();
  a_style_added_later_beats_the_start();
  the_start_beats_a_theme();
  the_start_is_no_local_style();
  what_removes_the_start();
  return failures;
}
