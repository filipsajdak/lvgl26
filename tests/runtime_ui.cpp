#include <string>
#include <algorithm>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

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

// What a document must not be able to do.
void rejects(lv::obj screen) {
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
  rejects(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
