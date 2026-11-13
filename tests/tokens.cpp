#include <lv/lv.hpp>

#include "check.hpp"

namespace {

using namespace lv::literals;

constexpr char dark_json[] = {
#embed "tokens.json"
    , 0};
constexpr char light_json[] = {
#embed "tokens_light.json"
    , 0};

constexpr auto dark = lv::tokens<dark_json>;
constexpr auto light = lv::tokens<light_json>;

// Types: every token is the unit type its $type asks for.
static_assert(std::is_same_v<decltype(dark.color.primary), lv_color_t>);
static_assert(std::is_same_v<decltype(dark.space.md), lv::length>);
static_assert(std::is_same_v<decltype(dark.motion.fast), lv::duration>);
static_assert(std::is_same_v<decltype(dark.font.family), std::string_view>);

// Values: hex strings, the object form, px, rem, ms and s.
static_assert(dark.color.palette.blue_500.red == 0x33 && dark.color.palette.blue_500.blue == 0xff);
static_assert(dark.color.text.red == 0xe6 && dark.color.text.green == 0xed);
static_assert(dark.space.sm == 4_px && dark.space.md == 12_px && dark.space.lg == 24_px);
static_assert(dark.motion.fast == 150_ms && dark.motion.slow == 400_ms);
static_assert(dark.font.family == "Montserrat" && dark.font.weight == 700);
static_assert(dark.font.scale == 1.25);

// Aliases, including one that crosses groups and one that inherits its type.
static_assert(dark.color.primary.green == dark.color.palette.blue_500.green);
static_assert(dark.radius.pill == dark.space.lg);

// A group whose tokens are all of an unsupported type (shadow) is an empty struct.
static_assert(std::is_empty_v<decltype(dark.elevation)>);

// One screen, written once, themed by a token set.
template <const auto& T>
constexpr auto themed = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320_px, 240_px},
     .style = {.bg_color = T.color.surface, .bg_opa = 100_pct, .radius = 0_px}})[
    lv::column({.id = "card",
                .size = {200_px, lv::size_content},
                .style = {.bg_color = T.color.card, .bg_opa = 100_pct, .border_width = 0_px,
                          .pad_all = T.space.md, .radius = T.radius.card}})[
        lv::label({.style = {.text_color = T.color.text}, .text = "Speed"}),
        lv::label({.style = {.text_color = T.color.primary}, .text = "120 km/h"})]];

bool same(lv_color_t a, lv_color_t b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

void applies_tokens(lv::obj screen) {
  auto d = lv::mount<themed<dark>>(screen);
  auto l = lv::mount<themed<light>>(screen);
  CHECK(d.has_value() && l.has_value());
  if (!d || !l) return;

  auto dc = d->get<"card">();
  auto lc = l->get<"card">();
  CHECK(same(dc.get<"style_bg_color">(LV_PART_MAIN), dark.color.card));
  CHECK(same(lc.get<"style_bg_color">(LV_PART_MAIN), light.color.card));
  CHECK(dc.get<"style_radius">(LV_PART_MAIN) == 8);
  CHECK(lc.get<"style_radius">(LV_PART_MAIN) == 20);
  CHECK(dc.get<"style_pad_left">(LV_PART_MAIN) == 12);
}

// Changing the tokens changes the pixels. Each theme is compared with its own
// reference first, so that a missing reference is never written from the wrong theme.
void renders(lv::obj screen, const char* dark_reference, const char* light_reference) {
  {
    auto root = lv::mount<themed<dark>>(screen);
    CHECK(lv_test_screenshot_compare(dark_reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  }
  auto root = lv::mount<themed<light>>(screen);
  CHECK(lv_test_screenshot_compare(light_reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  CHECK(lv_test_screenshot_compare(dark_reference) != LV_TEST_SCREENSHOT_RESULT_PASSED);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::printf("usage: tokens <dark.png> <light.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  applies_tokens(screen);
  renders(screen, argv[1], argv[2]);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
