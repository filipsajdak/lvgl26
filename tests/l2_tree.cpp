#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>
#include <type_traits>

namespace {

using namespace lv::literals;

int resets = 0;
void reset() { ++resets; }

// The slice screen: a title, a dial and a reset button, in a column.
constexpr auto ui = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320_px, 240_px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .size = {110_px, 110_px}, .value = 120}),
    lv::button({.id = "reset", .on_clicked = &reset})[lv::label({.text = "Reset"})]];

static_assert(lv::detail::id_count<ui> == 3);

// The typed values a props struct accepts.
static_assert(std::is_same_v<decltype(lv::props<"obj">{}.width), lv::arg<lv::length>>);
static_assert(std::is_same_v<decltype(lv::props<"arc">{}.rotation), lv::arg<lv::angle>>);
static_assert(std::is_same_v<decltype(lv::props<"arc">{}.value), lv::arg<lv::strict<std::int32_t>>>);
static_assert(std::is_same_v<decltype(lv::props<"label">{}.text), lv::arg<const char*>>);
static_assert(std::is_same_v<decltype(lv::props<"label">{}.on_clicked), lv::arg<lv::handler>>);
static_assert(lv::length{50_pct}.raw() == LV_PCT(50));
static_assert(lv::opa{50_pct}.raw() == 127);
static_assert((0x3366ff_rgb).red == 0x33 && (0x3366ff_rgb).blue == 0xff);

void builds_and_names(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  CHECK(root.has_value());
  if (!root) return;

  auto title = root->get<"title">();
  auto speedo = root->get<"speedo">();
  static_assert(std::is_same_v<decltype(title), lv::view<"label">>);
  static_assert(std::is_same_v<decltype(speedo), lv::view<"arc">>);

  CHECK(std::strcmp(title.get<"text">(), "Speed") == 0);
  CHECK(speedo.get<"value">() == 120);
  CHECK(speedo.get<"max_value">() == 240);
  CHECK(lv_obj_get_child_count(root->root().raw()) == 3);
  CHECK(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);

  resets = 0;
  lv_obj_send_event(root->get<"reset">().raw(), LV_EVENT_CLICKED, nullptr);
  CHECK(resets == 1);

  speedo.set({.value = 60});
  CHECK(speedo.get<"value">() == 60);
}

void owns_its_widgets(lv::obj screen) {
  const auto before = lv_obj_get_child_count(screen.raw());
  {
    auto root = lv::mount<ui>(screen);
    CHECK(lv_obj_get_child_count(screen.raw()) == before + 1);
    auto moved = std::move(*root);
    CHECK(lv_obj_get_child_count(screen.raw()) == before + 1);
    CHECK(moved.get<"speedo">().get<"value">() == 120);
  }
  CHECK(lv_obj_get_child_count(screen.raw()) == before);

  // The parent deletes first: nothing is deleted twice.
  {
    auto box = screen.add<"obj">();
    auto root = lv::mount<ui>(box);
    lv_obj_delete(box.raw());
    CHECK(!root->root());
  }
  CHECK(lv_obj_get_child_count(screen.raw()) == before);
}

void renders(lv::obj screen, const char* reference) {
  auto root = lv::mount<ui>(screen);
  CHECK(lv_test_screenshot_compare(reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);

  // The comparison must notice a change.
  root->get<"speedo">().set({.value = 0});
  CHECK(lv_test_screenshot_compare(reference) != LV_TEST_SCREENSHOT_RESULT_PASSED);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: l2_tree <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  builds_and_names(screen);
  owns_its_widgets(screen);
  renders(screen, argv[1]);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
