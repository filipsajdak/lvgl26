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
      {"type": "obj", "id": "q", "props": {"style": {"anim_duration": "300ms"}}}]})",
                       screen);
  check(more.has_value());
  if (more) {
    check(lv::view<"arc">{more->find("a").raw()}.get<"rotation">() == 90);
    check(more->find("o").get<"style_anim_duration">(LV_PART_MAIN) == 500);
    check(more->find("p").get<"style_anim_duration">(LV_PART_MAIN) == 250);
    check(more->find("q").get<"style_anim_duration">(LV_PART_MAIN) == 300);
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
  expect(R"({"type": "obj", "props": {"style": {"bg_color": "red"}}})", lv::load_error::bad_value, R"("red")");
  expect(R"({"type": "obj", "props": {"style": {"colour": "#000000"}}})", lv::load_error::unknown_property, "colour");
  expect(R"({"type": "arc", "props": {"rotation": "90rad"}})", lv::load_error::bad_value, R"("90rad")");
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
