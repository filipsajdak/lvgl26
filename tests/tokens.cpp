#include <lv/lv.hpp>

#include "check.hpp"

namespace {

using namespace lv::units;

constexpr char dark_json[] = {
#embed "tokens.json"
    , 0};
constexpr char light_json[] = {
#embed "tokens_light.json"
    , 0};

constexpr auto dark = lv::tokens<dark_json>;
constexpr auto light = lv::tokens<light_json>;

// Types: every token is an lv::token of the type its $type asks for.
static_assert(std::is_same_v<decltype(dark.color.primary), lv::token<lv_color_t>>);
static_assert(std::is_same_v<decltype(dark.space.md), lv::token<lv::length>>);
static_assert(std::is_same_v<decltype(dark.motion.slow), lv::token<lv::duration>>);
static_assert(std::is_same_v<decltype(dark.primitive.font.family), lv::token<std::string_view>>);
static_assert(std::is_same_v<decltype(dark.card.shadow), lv::token<lv::shadow_token>>);
static_assert(std::is_same_v<decltype(dark.text.title), lv::token<lv::typography_token>>);

// Primitive values: hex strings, the object form, px, rem, ms, s, numbers, lists.
static_assert(dark.primitive.color.blue_500->red == 0x33 && dark.primitive.color.blue_500->blue == 0xff);
static_assert(dark.primitive.color.paper->red == 0xe6 && dark.primitive.color.paper->green == 0xed);
static_assert(dark.primitive.space._1 == 4 * px && dark.primitive.space._3 == 12 * px);
static_assert(dark.primitive.space._6 == 24 * px);
static_assert(dark.primitive.duration.fast == 150 * ms && dark.primitive.duration.slow == 400 * ms);
static_assert(*dark.primitive.font.family == "Montserrat" && *dark.primitive.font.weight_bold == 700);
static_assert(*dark.primitive.font.scale == 1.25);
static_assert(*dark.primitive.easing.standard == lv::bezier{0.2, 0, 0, 1});

// Semantic tokens are aliases of primitives, also across groups.
static_assert(dark.color.primary->green == dark.primitive.color.blue_500->green);
static_assert(dark.space.lg == dark.primitive.space._6 && dark.motion.slow == 400 * ms);
static_assert(dark.card.padding == dark.space.md);  // an alias of an alias

// Composite tokens, with aliases inside them.
static_assert(dark.card.shadow->blur == 8 * px && dark.card.shadow->offset_y == 2 * px);
static_assert(dark.card.border->width == 1 * px && dark.card.border->style == "solid");
static_assert(dark.card.border->color.red == dark.color.outline->red);
static_assert(dark.text.title->font_size == 20 * px && dark.text.title->font_weight == 700);
static_assert(dark.text.title->font_family == "Montserrat" && dark.text.body->line_height == 1.4);
static_assert(dark.motion.enter->duration == 150 * ms && dark.motion.enter->timing.x1 == 0.2);
static_assert(dark.card.fill->count == 2 && dark.card.fill->stops[1].position == 1);

// A group whose tokens all have a $type this version does not know is an empty struct.
static_assert(std::is_empty_v<decltype(dark.unknown)>);

// One screen, written once, themed by a token set. Every style value is a token.
template <const auto& T>
constexpr auto themed = lv::column(
    {.flex_align = {LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER},
     .size = {320 * px, 240 * px},
     .style = {.bg_color = T.color.surface, .bg_opa = T.opacity.opaque, .radius = T.screen.radius}})[
    lv::column({.id = "card",
                .size = {200 * px, lv::size_content},
                .style = lv::style({.bg_color = T.color.card, .bg_opa = T.opacity.opaque,
                                    .pad_all = T.card.padding, .radius = T.card.radius}) |
                         T.card.border | T.card.shadow})[
        lv::label({.id = "title", .style = lv::style({.text_color = T.color.text}) | T.text.title,
                   .text = "Speed"}),
        lv::label({.style = {.text_color = T.color.primary, .text_opa = T.opacity.muted},
                   .text = "120 km/h"})]];

bool same(lv_color_t a, lv_color_t b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

void applies_tokens(lv::obj screen) {
  auto d = lv::mount<themed<dark>>(screen);
  auto l = lv::mount<themed<light>>(screen);
  check(d.has_value() && l.has_value());
  if (!d || !l) return;

  auto dc = d->get<"card">();
  auto lc = l->get<"card">();
  check(same(dc.get<"style_bg_color">(LV_PART_MAIN), dark.color.card));
  check(same(lc.get<"style_bg_color">(LV_PART_MAIN), light.color.card));
  check(dc.get<"style_radius">(LV_PART_MAIN) == 8);
  check(lc.get<"style_radius">(LV_PART_MAIN) == 20);
  check(dc.get<"style_pad_left">(LV_PART_MAIN) == 12);

  // The composite tokens merged with | reached the widget.
  check(dc.get<"style_border_width">(LV_PART_MAIN) == 1);
  check(same(dc.get<"style_border_color">(LV_PART_MAIN), dark.color.outline));
  check(dc.get<"style_shadow_width">(LV_PART_MAIN) == 8);
  check(dc.get<"style_shadow_offset_y">(LV_PART_MAIN) == 2);
  check(d->get<"title">().get<"style_text_letter_space">(LV_PART_MAIN) == 1);

  // lv::raw marks a deliberate exception and is accepted in strict mode.
  dc.set({.style = {.radius = lv::raw(3 * px)}});
  check(dc.get<"style_radius">(LV_PART_MAIN) == 3);
}

// Changing the tokens changes the pixels. Each theme is compared with its own
// reference first, so that a missing reference is never written from the wrong theme.
void renders(lv::obj screen, const char* dark_reference, const char* light_reference) {
  {
    auto root = lv::mount<themed<dark>>(screen);
    check(lv_test_screenshot_compare(dark_reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  }
  auto root = lv::mount<themed<light>>(screen);
  check(lv_test_screenshot_compare(light_reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  check(lv_test_screenshot_compare(dark_reference) != LV_TEST_SCREENSHOT_RESULT_PASSED);
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
