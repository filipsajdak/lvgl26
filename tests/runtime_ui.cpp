#include <string>
#include <algorithm>
#include <lv/lv.hpp>
#include <lv/runtime.hpp>

#include "check.hpp"

#include <chrono>
#include <cstring>
#include <limits>

namespace {

using namespace lv::units;

// The slice screen, once as a compile-time tree and once as a document.
constexpr auto tree = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .size = {110 * px, 110 * px}, .value = 120}),
    lv::button({.id = "reset"})[lv::label({.text = "Reset"})]];

constexpr std::string_view document = R"({
  "type": "column",
  "props": {"flex_align": ["center", "center", "LV_FLEX_ALIGN_CENTER"], "size": ["320px", "240px"]},
  "children": [
    {"type": "label", "id": "title", "props": {"text": "Speed"}},
    {"type": "arc", "id": "speedo", "props": {"range": [0, 240], "size": [110, 110], "value": 120}},
    {"type": "button", "id": "reset", "children": [
      {"type": "label", "props": {"text": "Reset"}}]}
  ]})";

using ui = lv::runtime<"obj", "label", "arc", "button">;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

// A picture of the active screen, for comparing two ways of building it.
struct picture {
  lv_draw_buf_t* buf;
  explicit picture(lv_draw_buf_t* b) : buf(b) {}
  picture(const picture&) = delete;
  picture& operator=(const picture&) = delete;
  ~picture() {
    if (buf) lv_draw_buf_destroy(buf);
  }
  bool same_as(const picture& other) const {
    if (!buf || !other.buf) return false;
    const auto size = static_cast<std::size_t>(buf->header.stride) * buf->header.h;
    return std::memcmp(buf->data, other.buf->data, size) == 0;
  }
};

picture take() { return picture{lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888)}; }

void same_pixels_as_the_tree(lv::obj screen) {
  lv_draw_buf_t* from_tree = nullptr;
  {
    auto mounted = lv::mount<tree>(screen);
    from_tree = lv_snapshot_take(screen.raw(), LV_COLOR_FORMAT_ARGB8888);
  }
  const picture expected{from_tree};

  auto loaded = ui::load(document, screen);
  check(loaded.has_value());
  if (!loaded) return;
  check(take().same_as(expected));

  // Ids, and changing properties by id.
  auto speedo = lv::view<"arc">{loaded->find("speedo").raw()};
  check(speedo && speedo.get<"value">() == 120 && speedo.get<"max_value">() == 240);
  check(!loaded->find("nope"));
  check(loaded->apply(R"({"speedo": {"value": 60}, "title": {"text": "Trip"}})").has_value());
  check(speedo.get<"value">() == 60);
  check(std::strcmp(lv::view<"label">{loaded->find("title").raw()}.get<"text">(), "Trip") == 0);
  check(!take().same_as(expected));
}

void styles_and_units(lv::obj screen) {
  auto loaded = ui::load(R"({"type": "obj", "id": "box", "props": {
      "size": ["50%", "content"], "align": "center",
      "style": {"bg_color": "#112233", "bg_opa": "50%", "radius": "8px", "border_width": 2},
      "style_pressed": {"bg_color": "#445566"}}})",
                         screen);
  check(loaded.has_value());
  if (!loaded) return;
  auto box = loaded->find("box");
  const auto c = box.get<"style_bg_color">(LV_PART_MAIN);
  check(c.red == 0x11 && c.green == 0x22 && c.blue == 0x33);
  check(box.get<"style_radius">(LV_PART_MAIN) == 8);
  check(box.get<"style_bg_opa">(LV_PART_MAIN) == 127);
  check(box.get<"style_border_width">(LV_PART_MAIN) == 2);
  check(box.get<"style_align">(LV_PART_MAIN) == LV_ALIGN_CENTER);

  // Angles and times, with and without their unit.
  auto more = ui::load(R"({"type": "column", "children": [
      {"type": "arc", "id": "a", "props": {"rotation": "90deg", "bg_angles": [10, "200deg"]}},
      {"type": "obj", "id": "o", "props": {"style": {"anim_duration": "0.5s"}}},
      {"type": "obj", "id": "p", "props": {"style": {"anim_duration": 250}}},
      {"type": "obj", "id": "q", "props": {"style": {"anim_duration": "300ms"}}},
      {"type": "obj", "id": "r", "props": {"style": {"transform_rotation": 22.5}}},
      {"type": "obj", "id": "s", "props": {"style": {"transform_rotation": "-45deg"}}},
      {"type": "obj", "id": "t", "props": {"style": {"transform_skew_x": "0.1deg"}}}]})",
                       screen);
  check(more.has_value());
  if (more) {
    check(lv::view<"arc">{more->find("a").raw()}.get<"rotation">() == 90 * deg);
    check(more->find("o").get<"style_anim_duration">(LV_PART_MAIN) == 500);
    check(more->find("p").get<"style_anim_duration">(LV_PART_MAIN) == 250);
    check(more->find("q").get<"style_anim_duration">(LV_PART_MAIN) == 300);
    // Where LVGL takes tenths of a degree, a document may give one decimal.
    check(lv_obj_get_style_transform_rotation(more->find("r").raw(), LV_PART_MAIN) == 225);
    check(more->find("r").get<"style_transform_rotation">(LV_PART_MAIN) == 22.5 * deg);
    check(more->find("s").get<"style_transform_rotation">(LV_PART_MAIN) == -45 * deg);
    check(lv_obj_get_style_transform_skew_x(more->find("t").raw(), LV_PART_MAIN) == 1);
  }
}

// What a document may set as text, and which kinds of value a property can be read as.
using lv::detail::readable;
static_assert(readable<lv::length> && readable<lv::horizontal> && readable<lv::vertical> && readable<lv::angle>);
static_assert(readable<lv::decidegrees> && readable<lv::duration> && readable<lv::opa> && readable<lv_color_t>);
static_assert(readable<const char*> && readable<lv::option_list> && readable<lv_align_t>);
static_assert(readable<lv::strict<int>> && readable<lv::styled<lv::length>> && readable<lv::nonnegative<lv::length>>);
static_assert(!readable<int> && !readable<double> && !readable<bool> && !readable<std::string_view>);
static_assert(!readable<lv_obj_t*> && !readable<lv::styled<int>>);

using list_ui = lv::runtime<"obj", "dropdown", "roller", "slider", "arc", "button", "switch">;

// A list of options is copied into the buffer next to it, up to one less than its size.
void option_lists_are_copied(lv::obj screen) {
  const auto dropdown_with = [](std::size_t n) {
    return R"({"type": "dropdown", "id": "d", "props": {"options": ")" + std::string(n, 'o') + R"("}})";
  };
  auto fits = list_ui::load(dropdown_with(lv::detail::text_capacity - 1), screen);
  check(fits.has_value());
  if (fits) check(std::strlen(lv_dropdown_get_options(fits->find("d").raw())) == lv::detail::text_capacity - 1);
  check(!list_ui::load(dropdown_with(lv::detail::text_capacity), screen).has_value());
  check(!list_ui::load(dropdown_with(lv::detail::text_capacity + 1), screen).has_value());
  check(!list_ui::load(R"({"type": "dropdown", "props": {"options": 3}})", screen).has_value());
  check(!list_ui::load(R"({"type": "dropdown", "props": {"options": ["a", "b"]}})", screen).has_value());

  // The whole text, a short one too, and a roller's as well.
  auto both = list_ui::load(R"({"type": "column", "children": [
      {"type": "dropdown", "id": "d", "props": {"options": "ab cd"}},
      {"type": "roller", "id": "r", "props": {"options": ["xy", "normal"]}},
      {"type": "dropdown", "id": "one", "props": {"options": "z"}}]})", screen);
  check(both.has_value());
  if (both) {
    check(std::strcmp(lv_dropdown_get_options(both->find("d").raw()), "ab cd") == 0);
    check(std::strcmp(lv_roller_get_options(both->find("r").raw()), "xy") == 0);
    check(std::strcmp(lv_dropdown_get_options(both->find("one").raw()), "z") == 0);
  }
}

// Only a column and a row are flex containers; every other kind of widget is not, whatever its name sorts as.
void only_a_column_and_a_row_lay_out(lv::obj screen) {
  auto widgets = list_ui::load(R"({"type": "obj", "children": [
      {"type": "arc", "id": "arc"}, {"type": "button", "id": "button"}, {"type": "dropdown", "id": "dropdown"},
      {"type": "obj", "id": "obj"}, {"type": "roller", "id": "roller"}, {"type": "slider", "id": "slider"},
      {"type": "switch", "id": "switch"}]})", screen);
  check(widgets.has_value());
  if (!widgets) return;
  for (const char* id : {"arc", "button", "dropdown", "obj", "roller", "slider", "switch"})
    check(widgets->find(id).get<"style_layout">(LV_PART_MAIN) != LV_LAYOUT_FLEX);
}

// What a document must not be able to do.
void rejects(lv::obj screen, bool holds_heap) {
  const auto before = lv_obj_get_child_count(screen.raw());
  const auto heap = free_bytes();
  const auto expect = [&](std::string_view doc, lv::load_error error, std::string_view at) {
    const auto r = ui::load(doc, screen);
    check(!r.has_value());
    if (!r) {
      check(r.error().error == error);
      check(r.error().at == at);
    }
    check(lv_obj_get_child_count(screen.raw()) == before);  // nothing stays behind
  };

  expect(R"({"type": "label")", lv::load_error::malformed, R"({"type": "label")");
  expect(R"({"props": {}})", lv::load_error::malformed, R"({"props": {}})");
  expect(R"({"type": "chart"})", lv::load_error::unknown_widget, "chart");            // exists in LVGL, not allowed here
  expect(R"({"type": "label", "props": {"txt": "x"}})", lv::load_error::unknown_property, "txt");
  expect(R"({"type": "arc", "props": {"value": "many"}})", lv::load_error::bad_value, R"("many")");
  expect(R"({"type": "arc", "props": {"value": 1.5}})", lv::load_error::bad_value, "1.5");
  expect(R"({"type": "arc", "props": {"value": true}})", lv::load_error::bad_value, "true");
  expect(R"({"type": "arc", "props": {"range": [0]}})", lv::load_error::bad_value, "[0]");
  expect(R"({"type": "arc", "props": {"range": [0, 240, 5]}})", lv::load_error::bad_value, "[0, 240, 5]");
  expect(R"({"type": "obj", "props": {"width": "12em"}})", lv::load_error::bad_value, R"("12em")");
  expect(R"({"type": "obj", "props": {"align": "middle"}})", lv::load_error::bad_value, R"("middle")");
  expect(R"({"type": "obj", "props": {"align": "centxr"}})", lv::load_error::bad_value, R"("centxr")");
  // A unit needs its number; a color needs '#' and six hex digits.
  expect(R"({"type": "arc", "props": {"rotation": "deg"}})", lv::load_error::bad_value, R"("deg")");
  expect(R"({"type": "obj", "props": {"width": "px"}})", lv::load_error::bad_value, R"("px")");
  expect(R"({"type": "obj", "props": {"style": {"anim_duration": "ms"}}})", lv::load_error::bad_value, R"("ms")");
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "x112233"}}})", lv::load_error::bad_value, R"("x112233")");
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "#11223g"}}})", lv::load_error::bad_value, R"("#11223g")");
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "!112233"}}})", lv::load_error::bad_value, R"("!112233")");
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "#1122334"}}})", lv::load_error::bad_value, R"("#1122334")");
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "#11223"}}})", lv::load_error::bad_value, R"("#11223")");
  expect(R"({"type": "obj", "props": {"width": "5$"}})", lv::load_error::bad_value, R"("5$")");
  expect(R"({"type": "obj", "props": {"width": "5!"}})", lv::load_error::bad_value, R"("5!")");
  {
    // "row" and "column" are flex containers in that direction; other widgets are not.
    auto flows = ui::load(R"({"type": "row", "id": "r", "children": [
        {"type": "column", "id": "c"}, {"type": "label", "id": "s"}]})", screen);
    check(flows.has_value());
    if (flows) {
      check(flows->find("r").get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
      check(flows->find("c").get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
      check(flows->find("r").get<"style_layout">(LV_PART_MAIN) == LV_LAYOUT_FLEX);
      check(flows->find("s").get<"style_layout">(LV_PART_MAIN) != LV_LAYOUT_FLEX);
    }
  }
  {
    // Text up to one less than the buffer is taken; at the buffer size it is refused.
    const auto label_with = [](std::size_t n) {
      return R"({"type": "label", "id": "t", "props": {"text": ")" + std::string(n, 'x') + R"("}})";
    };
    auto fits = ui::load(label_with(lv::detail::text_capacity - 1), screen);
    check(fits.has_value());
    if (fits)
      check(std::strlen(lv::view<"label">{fits->find("t").raw()}.get<"text">()) ==
            lv::detail::text_capacity - 1);
    check(!ui::load(label_with(lv::detail::text_capacity), screen).has_value());
    check(!ui::load(label_with(lv::detail::text_capacity + 1), screen).has_value());

    // Zero is a time; black is a color; a screen without ids has no widget called "".
    auto zero = ui::load(R"({"type": "obj", "id": "z", "props": {"style":
        {"anim_duration": 0, "bg_color": "#00000A", "text_color": "#000000"}}})", screen);
    check(zero.has_value());
    if (zero) {
      check(zero->find("z").get<"style_anim_duration">(LV_PART_MAIN) == 0);
      check(lv_color_eq(zero->find("z").get<"style_bg_color">(LV_PART_MAIN), lv_color_hex(0x00000A)));
    }
    // A loaded screen keeps its ids after the document's text is gone. (The label
    // above was loaded from a temporary string.) An id has a length limit.
    const auto with_id = [](std::size_t n) {
      return R"({"type": "obj", "id": ")" + std::string(n, 'i') + R"("})";
    };
    auto longest = ui::load(with_id(ui::id_length), screen);
    check(longest.has_value() && longest->find(std::string(ui::id_length, 'i')));
    check(longest.has_value() && !longest->find(std::string(ui::id_length - 1, 'i')));
    const auto too_long = ui::load(with_id(ui::id_length + 1), screen);
    check(!too_long.has_value() && too_long.error().error == lv::load_error::id_too_long);
    // An id is a string that is not empty.
    for (const char* bad : {R"({"type": "obj", "id": ""})", R"({"type": "obj", "id": 5})"}) {
      const auto refused = ui::load(bad, screen);
      check(!refused.has_value() && refused.error().error == lv::load_error::malformed);
    }
    // Exactly as many ids as a screen keeps are taken, and all of them are found;
    // one more is refused.
    const auto with_ids = [](std::size_t n) {
      std::string doc = R"({"type": "column", "children": [)";
      for (std::size_t i = 0; i < n; ++i)
        doc += (i ? "," : "") + (R"({"type": "obj", "id": "w)" + std::to_string(i) + R"("})");
      return doc + "]}";
    };
    auto full = ui::load(with_ids(ui::id_capacity), screen);
    check(full.has_value());
    if (full) {
      check(full->find("w0") && full->find("w" + std::to_string(ui::id_capacity - 1)));
      check(!full->find("w" + std::to_string(ui::id_capacity)) && !full->find(""));
    }
    const auto over = ui::load(with_ids(ui::id_capacity + 1), screen);
    check(!over.has_value() && over.error().error == lv::load_error::too_many_ids);
    auto anonymous = ui::load(R"({"type": "obj"})", screen);
    check(anonymous.has_value() && !anonymous->find(""));
  }
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "red"}}})", lv::load_error::bad_value, R"("red")");
  expect(R"({"type": "obj", "props": {"style": {"colour": "#000000"}}})", lv::load_error::unknown_property, "colour");
  expect(R"({"type": "arc", "props": {"rotation": "90rad"}})", lv::load_error::bad_value, R"("90rad")");
  // An angle in tenths of a degree: not finer than a tenth, not another unit, not
  // more than fits, and not a word.
  expect(R"({"type": "obj", "props": {"style": {"transform_rotation": 22.55}}})", lv::load_error::bad_value, "22.55");
  expect(R"({"type": "obj", "props": {"style": {"transform_rotation": "22.5rad"}}})", lv::load_error::bad_value, R"("22.5rad")");
  expect(R"({"type": "obj", "props": {"style": {"transform_rotation": 1e12}}})", lv::load_error::bad_value, "1e12");
  expect(R"({"type": "obj", "props": {"style": {"transform_rotation": "deg"}}})", lv::load_error::bad_value, R"("deg")");
  expect(R"({"type": "obj", "props": {"style": {"transform_rotation": true}}})", lv::load_error::bad_value, "true");
  expect(R"({"type": "obj", "props": {"style": {"anim_duration": "5min"}}})", lv::load_error::bad_value, R"("5min")");
  expect(R"({"type": "obj", "props": {"style": {"anim_duration": -5}}})", lv::load_error::bad_value, "-5");
  expect(R"({"type": "obj", "props": {"style": {"bg_opa": 0.5}}})", lv::load_error::bad_value, "0.5");
  // Handlers and pointers cannot come from a document.
  expect(R"({"type": "button", "props": {"on_clicked": "reset"}})", lv::load_error::unknown_property, "on_clicked");
  expect(R"({"type": "obj", "props": {"user_data": 1}})", lv::load_error::unknown_property, "user_data");
  // A text setter that keeps the pointer it is given must not be reachable: the
  // document's text is gone after the call. The copying one works.
  expect(R"({"type": "label", "props": {"text_static": "x"}})", lv::load_error::unknown_property, "text_static");
  expect(R"({"type": "column", "children": [{"type": "label", "id": "a"}, {"type": "arc", "id": "a"}]})",
         lv::load_error::duplicate_id, "a");
  // A failure deep in the tree removes the widgets built before it.
  expect(R"({"type": "column", "children": [{"type": "label"}, {"type": "arc", "props": {"nope": 1}}]})",
         lv::load_error::unknown_property, "nope");

  auto ok = ui::load(document, screen);
  check(ok.has_value());
  if (ok) {
    const auto r = ok->apply(R"({"missing": {"value": 1}})");
    check(!r && r.error().error == lv::load_error::unknown_id && r.error().at == "missing");
    const auto bad = ok->apply(R"({"speedo": {"value": "x"}})");
    check(!bad && bad.error().error == lv::load_error::bad_value);

    // A widget that was deleted behind the loader's back is not touched again, and
    // the ids of the others still reach the right widgets although positions moved.
    lv_obj_delete(ok->find("title").raw());
    check(!ok->find("title"));
    const auto gone = ok->apply(R"({"title": {"text": "x"}})");
    check(!gone && gone.error().error == lv::load_error::unknown_id);
    check(ok->apply(R"({"speedo": {"value": 33}})").has_value());
    check(lv::view<"arc">{ok->find("speedo").raw()}.get<"value">() == 33);
  }
  ok = {};
  if (holds_heap) check(free_bytes() == heap);
}

// Widgets whose setters take unsigned numbers, or allocate once per row.
using counted = lv::runtime<"table", "scale", "chart">;

// Expects `doc` to be refused as a bad value at `at`, leaving nothing behind.
void expect_bad_value(std::string_view doc, std::string_view at, lv::obj screen) {
  const auto before = lv_obj_get_child_count(screen.raw());
  const auto r = counted::load(doc, screen);
  check(!r.has_value() && r.error().error == lv::load_error::bad_value && r.error().at == at);
  check(lv_obj_get_child_count(screen.raw()) == before);
}

// A table's rows and columns are limited: LVGL allocates rows times columns cells
// and does not report a failed allocation, so 4000000000 rows crashed the next layout.
void limit_table_rows_and_columns(lv::obj screen) {
  const auto heap = free_bytes();
  constexpr auto most = lv::detail::most_table_rows_or_columns;
  const std::string n = std::to_string(most);
  const std::string over = std::to_string(most + 1);
  {
    auto at_limit = counted::load(R"({"type": "table", "id": "t", "props": {"row_count": )" + n +
                                      R"(, "column_count": )" + n + R"(, "column_width": [)" + n +
                                      R"(, "10px"], "cell_ctrl": [)" + n + R"(, )" + n +
                                      R"(, "merge_right"]}})",
                                  screen);
    check(at_limit.has_value());
    if (at_limit) {
      lv_obj_update_layout(screen.raw());
      // An index at the limit adds one row or column past it.
      check(lv_table_get_row_count(at_limit->find("t").raw()) == most + 1);
      check(lv_table_get_column_count(at_limit->find("t").raw()) == most + 1);
      const auto patched = at_limit->apply(R"({"t": {"row_count": )" + over + "}}");
      check(!patched && patched.error().error == lv::load_error::bad_value);
      check(lv_table_get_row_count(at_limit->find("t").raw()) == most + 1);
    }
  }
  {
    // The limit is on the counts and indices, not on a width beside them.
    auto widths = counted::load(R"({"type": "table", "id": "t", "props": {"column_count": 2,
        "column_width": [0, "100px"]}})", screen);
    check(widths.has_value() && lv_table_get_column_width(widths->find("t").raw(), 0) == 100);
    if (widths) check(widths->apply(R"({"t": {"column_width": [1, "50%"]}})").has_value());
  }
  expect_bad_value(R"({"type": "table", "props": {"row_count": )" + over + "}}", over, screen);
  expect_bad_value(R"({"type": "table", "props": {"row_count": 4000000000}})", "4000000000", screen);
  expect_bad_value(R"({"type": "table", "props": {"column_count": )" + over + "}}", over, screen);
  expect_bad_value(R"({"type": "table", "props": {"column_width": [)" + over + R"(, "10px"]}})",
                   "[" + over + R"(, "10px"])", screen);
  expect_bad_value(R"({"type": "table", "props": {"cell_ctrl": [0, )" + over + R"(, "merge_right"]}})",
                   "[0, " + over + R"(, "merge_right"])", screen);
  expect_bad_value(R"({"type": "table", "props": {"cell_ctrl": [)" + over + R"(, 0, "merge_right"]}})",
                   "[" + over + R"(, 0, "merge_right"])", screen);
  check(free_bytes() == heap);
}

// A chart's points are limited. A document cannot add a series, but its program
// can: then LVGL reallocates the series to the new count, and when that failed
// (100000000 points) the next refresh read the null array.
void limit_chart_points(lv::obj screen) {
  constexpr auto most = lv::detail::most_chart_points;
  // The limit the header documents: a chart takes 1024 points, a table 64 rows.
  check(counted::load(R"({"type": "chart", "props": {"point_count": 1024}})", screen).has_value());
  check(counted::load(R"({"type": "table", "props": {"row_count": 64, "column_count": 64}})", screen).has_value());
  auto chart = counted::load(R"({"type": "chart", "id": "c", "props": {"size": ["300px", "100px"],
      "point_count": )" + std::to_string(most) + "}}",
                             screen);
  check(chart.has_value());
  if (!chart) return;
  check(lv_chart_add_series(chart->find("c").raw(), lv_color_hex(0xff0000), LV_CHART_AXIS_PRIMARY_Y) != nullptr);
  lv_refr_now(nullptr);
  for (const std::string& count : {std::to_string(most + 1), std::string("100000000")}) {
    const std::string patch = R"({"c": {"point_count": )" + count + "}}";
    const auto r = chart->apply(patch);  // the failure is a view into the patch
    check(!r && r.error().error == lv::load_error::bad_value && r.error().at == count);
  }
  lv_refr_now(nullptr);
  check(lv_chart_get_point_count(chart->find("c").raw()) == most);
}

// The other counts need no limit, and get none. LVGL keeps a scale's ticks in 15
// bits and draws at most 32767 division lines of a chart (an int16_t loop), so
// the largest counts draw without a crash, in a fraction of a second.
void capability_other_counts_take_their_whole_type(lv::obj screen) {
  lv_refr_now(nullptr);  // the first refresh allocates what every later one reuses
  const auto heap = free_bytes();
  for (const char* most : {"32767", "4294967295"}) {
    const std::string n = most;
    auto chart = counted::load(R"({"type": "chart", "id": "c", "props": {"size": ["300px", "100px"],
        "div_line_count": [)" + n + ", " + n + "]}}",
                               screen);
    auto scale = counted::load(R"({"type": "scale", "id": "s", "props": {"size": ["300px", "60px"],
        "total_tick_count": )" + n + R"(, "major_tick_every": )" + n + "}}",
                               screen);
    check(chart.has_value() && scale.has_value());
    if (chart && scale) {
      lv_refr_now(nullptr);  // lays out and draws both
      check(std::to_string(lv_chart_get_hor_div_line_count(chart->find("c").raw())) == n);
    }
  }
  check(free_bytes() == heap);
}

// A setter's parameter type is checked, not only the property's: a negative angle
// for lv_scale_set_angle_range(uint32_t) would arrive as about four billion.
// Without the check such a widget did not even compile in an lv::runtime.
void defect_unsigned_parameter_refuses_negative(lv::obj screen) {
  auto fits = counted::load(R"({"type": "scale", "id": "s", "props": {"angle_range": 270}})", screen);
  check(fits.has_value() && lv_scale_get_angle_range(fits->find("s").raw()) == 270);
  expect_bad_value(R"({"type": "scale", "props": {"angle_range": -1}})", "-1", screen);
  expect_bad_value(R"({"type": "scale", "props": {"angle_range": "-90deg"}})", R"("-90deg")", screen);
}

// A document is read in one pass. Each loop over members or children used to
// count them and then find number i from the start: 10000 members of "props"
// took 141 s in the Debug build. Now that takes well under a second, so the
// bound below holds on a loaded machine and under the sanitizers, and the old
// code misses it by minutes.
void limit_document_is_read_in_linear_time(lv::obj screen) {
  constexpr std::size_t members = 20000;
  constexpr std::size_t children = 2000;
  constexpr auto bound = std::chrono::seconds{10};
  std::string props;
  for (std::size_t i = 0; i < members; ++i) props += i == 0 ? R"("value": 1)" : R"(, "value": 1)";
  std::string kids;
  for (std::size_t i = 0; i < children; ++i) kids += i == 0 ? R"({"type": "obj"})" : R"(, {"type": "obj"})";
  std::string patch;
  for (std::size_t i = 0; i < members; ++i) patch += i == 0 ? R"("a": {"value": 2})" : R"(, "a": {"value": 2})";
  const std::string doc = R"({"type": "arc", "id": "a", "props": {)" + props + R"(}, "children": [)" + kids + "]}";

  const auto start = std::chrono::steady_clock::now();
  auto loaded = ui::load(doc, screen);
  check(loaded.has_value());
  if (loaded) {
    check(lv_obj_get_child_count(loaded->root().raw()) == children);
    check(loaded->apply("{" + patch + "}").has_value());
    check(lv::view<"arc">{loaded->find("a").raw()}.get<"value">() == 2);
  }
  check(std::chrono::steady_clock::now() - start < bound);
}

// A number too large for the integer it becomes, or not a number at all, is a bad
// value. The cast of such a double is undefined: 1e99 for a width became
// INT32_MAX on Arm, and only -fsanitize=float-cast-overflow
// reported it. 0e999 is zero times infinity: NaN.
void defect_number_outside_integer_is_refused(lv::obj screen) {
  const auto heap = free_bytes();
  const auto refused = [&](std::string_view doc, std::string_view at) {
    const auto r = ui::load(doc, screen);
    check(!r.has_value() && r.error().error == lv::load_error::bad_value && r.error().at == at);
  };
  refused(R"({"type": "obj", "props": {"size": [1e99, "10px"]}})", R"([1e99, "10px"])");
  refused(R"({"type": "obj", "props": {"width": "99999999999999999999px"}})", R"("99999999999999999999px")");
  refused(R"({"type": "obj", "props": {"width": "-99999999999%"}})", R"("-99999999999%")");
  refused(R"({"type": "obj", "props": {"width": 0e999}})", "0e999");
  refused(R"({"type": "arc", "props": {"value": 1e99}})", "1e99");
  refused(R"({"type": "arc", "props": {"value": -1e99}})", "-1e99");
  refused(R"({"type": "arc", "props": {"value": 0e999}})", "0e999");
  refused(R"({"type": "arc", "props": {"value": 1e999}})", "1e999");
  refused(R"({"type": "arc", "props": {"value": 2147483648}})", "2147483648");
  refused(R"({"type": "arc", "props": {"value": -2147483649}})", "-2147483649");
  refused(R"({"type": "arc", "props": {"value": -1.5}})", "-1.5");  // a fraction below zero too
  refused(R"({"type": "arc", "props": {"rotation": -1e99}})", "-1e99");
  refused(R"({"type": "arc", "props": {"rotation": 0e999}})", "0e999");
  refused(R"({"type": "obj", "props": {"style": {"anim_duration": 0e999}}})", "0e999");
  refused(R"({"type": "obj", "props": {"style": {"anim_duration": "99999999999s"}}})", R"("99999999999s")");
  refused(R"({"type": "obj", "props": {"style": {"bg_opa": "99999999999%"}}})", R"("99999999999%")");
  // The extremes of the integer itself still fit.
  auto extremes = ui::load(R"({"type": "column", "children": [
      {"type": "arc", "id": "high", "props": {"range": [-2147483648, 2147483647], "value": 2147483647}},
      {"type": "arc", "id": "low", "props": {"range": [-2147483648, 2147483647], "value": -2147483648}}]})",
                           screen);
  check(extremes.has_value());
  if (extremes) {
    check(lv::view<"arc">{extremes->find("high").raw()}.get<"value">() == 2147483647);
    check(lv::view<"arc">{extremes->find("low").raw()}.get<"value">() == -2147483648);
  }
  extremes = {};
  check(free_bytes() == heap);
}

}  // namespace

int main() {
  // Enumerators are named in full or by their last part, in any letter case.
  static_assert(lv::detail::names_enumerator("center", "LV_ALIGN_CENTER"));
  static_assert(lv::detail::names_enumerator("TOP_MID", "LV_ALIGN_TOP_MID"));
  static_assert(lv::detail::names_enumerator("LV_ALIGN_CENTER", "LV_ALIGN_CENTER"));
  // Every character counts, and the name must follow an underscore.
  static_assert(!lv::detail::names_enumerator("centxr", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("centeR_", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("enter", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("XLV_ALIGN_CENTER", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("centeq", "LV_ALIGN_CENTER"));
  static_assert(!lv::detail::names_enumerator("centes", "LV_ALIGN_CENTER"));
  static_assert(lv::detail::lower_char('A') == 'a' && lv::detail::lower_char('Z') == 'z');
  static_assert(lv::detail::lower_char('@') == '@' && lv::detail::lower_char('[') == '[');
  // Hex digits, at each end of each range.
  static_assert(lv::detail::hex_value('0') == 0 && lv::detail::hex_value('9') == 9);
  static_assert(lv::detail::hex_value('a') == 10 && lv::detail::hex_value('f') == 15);
  static_assert(lv::detail::hex_value('A') == 10 && lv::detail::hex_value('F') == 15);
  static_assert(lv::detail::hex_value('g') == -1 && lv::detail::hex_value('`') == -1);
  static_assert(lv::detail::hex_value('/') == -1 && lv::detail::hex_value(':') == -1);
  // Rounding is to the nearest whole number, away from zero at one half.
  static_assert(lv::detail::rounded_i32(2.5) == 3 && lv::detail::rounded_i32(2.4) == 2);
  static_assert(lv::detail::rounded_i32(-1.5) == -2 && lv::detail::rounded_i32(-2.4) == -2);
  static_assert(lv::detail::rounded_i32(-0.4) == 0 && lv::detail::rounded_i32(0) == 0);
  static_assert(lv::detail::rounded_i32(0.6) == 1 && lv::detail::rounded_i32(0.4) == 0);
  // A cast to an integer is defined when the whole part fits, and never for NaN.
  static_assert(lv::detail::castable_to<std::int32_t>(2147483647.9));
  static_assert(!lv::detail::castable_to<std::int32_t>(2147483648.0));
  static_assert(lv::detail::castable_to<std::int32_t>(-2147483648.9));
  static_assert(!lv::detail::castable_to<std::int32_t>(-2147483649.0));
  static_assert(lv::detail::castable_to<std::uint32_t>(-0.9) && !lv::detail::castable_to<std::uint32_t>(-1.0));
  static_assert(lv::detail::castable_to<std::uint32_t>(4294967295.0));
  static_assert(!lv::detail::castable_to<std::uint32_t>(4294967296.0));
  static_assert(!lv::detail::castable_to<std::int32_t>(std::numeric_limits<double>::quiet_NaN()));
  static_assert(!lv::detail::castable_to<std::int32_t>(std::numeric_limits<double>::infinity()));
  static_assert(!lv::detail::castable_to<std::int32_t>(-std::numeric_limits<double>::infinity()));
  static_assert(lv::detail::rounded_i32(2147483646.5) == 2147483647);
  static_assert(!lv::detail::rounded_i32(2147483647.5));
  static_assert(lv::detail::rounded_i32(-2147483647.5) == -2147483648);
  static_assert(!lv::detail::rounded_i32(-2147483648.5));
  // The tables of properties have no empty slot either.
  static_assert(std::ranges::none_of(lv::detail::property_table<"obj">,
                                     [](const auto& e) { return e.name.empty(); }));
  static_assert(std::ranges::none_of(lv::detail::property_table<"arc">,
                                     [](const auto& e) { return e.name.empty(); }));
  // The table of style properties has no empty slot.
  static_assert(!lv::detail::style_slots<>.empty());
  static_assert(std::ranges::none_of(lv::detail::style_slots<>,
                                     [](const auto& slot) { return slot.name.empty(); }));
  static_assert(!lv::detail::names_enumerator("enter", "LV_ALIGN_CENTER"));

  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  same_pixels_as_the_tree(screen);
  styles_and_units(screen);
  // The first run allocates a few bytes once, so only the second is held to the heap it began with.
  rejects(screen, false);
  rejects(screen, true);
  option_lists_are_copied(screen);
  only_a_column_and_a_row_lay_out(screen);
  limit_table_rows_and_columns(screen);
  limit_chart_points(screen);
  capability_other_counts_take_their_whole_type(screen);
  defect_unsigned_parameter_refuses_negative(screen);
  defect_number_outside_integer_is_refused(screen);
  limit_document_is_read_in_linear_time(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
