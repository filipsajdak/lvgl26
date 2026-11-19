#include <string>
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

// An angle token is a number of degrees. A style value in tenths of a degree takes
// one with a decimal; one in whole degrees takes a whole number.
static_assert(*dark.primitive.angle.tilt == 22.5 && *dark.primitive.angle.quarter == 90);
static_assert(lv::styled<lv::decidegrees>{dark.primitive.angle.tilt}.raw() == 225);
static_assert(lv::styled<lv::decidegrees>{dark.primitive.angle.quarter}.raw() == 900);
static_assert(lv::styled<lv::angle>{dark.primitive.angle.quarter}.raw() == 90);
static_assert(lv::style({.transform_rotation = dark.primitive.angle.tilt}).transform_rotation.v.v0 == 225);

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

// The value readers on their own, for the forms the token files above do not use.
constexpr bool is_rgb(lv_color_t c, int r, int g, int b) {
  return c.red == r && c.green == g && c.blue == b;
}
static_assert(is_rgb(lv::detail::color_from_hex("#AbCdEf"), 0xab, 0xcd, 0xef));
static_assert(is_rgb(lv::detail::color_from_hex("#09afAF"), 0x09, 0xaf, 0xaf));
static_assert(is_rgb(lv::detail::color_from_hex("#fa0"), 0xff, 0xaa, 0x00));  // #rgb doubles each digit
static_assert(lv::detail::rounded(2.5) == 3 && lv::detail::rounded(2.4) == 2);
static_assert(lv::detail::rounded(-1.5) == -2 && lv::detail::rounded(-2.4) == -2);
static_assert(lv::detail::rounded(0.6) == 1 && lv::detail::rounded(-0.4) == 0);
consteval lv_color_t color_token(std::string_view text) {
  const auto v = *lv::json::value::parse(text);
  return lv::detail::read_token(lv::detail::read_as<lv_color_t>{}, v, v);
}
static_assert(is_rgb(color_token(R"({"components": [1, 0.5, 0]})"), 255, 128, 0));
static_assert(is_rgb(color_token(R"({"hex": "#102030"})"), 0x10, 0x20, 0x30));
static_assert(is_rgb(color_token(R"("#102030")"), 0x10, 0x20, 0x30));

// The forms from before the Design Tokens Format 2025.10 are read only when the settings allow them,
// and give the same values as the object forms of tokens.json (the token file was rewritten by tools/dtcg_fix.py).
struct legacy_settings {};
}  // namespace
template <>
inline constexpr lv::config lv::cfg<legacy_settings>{.rules = {.tokens_legacy_values = true}};
namespace {

constexpr bool same_color(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

constexpr char legacy_json[] = {
#embed "tokens_legacy.json"
    , 0};
constexpr auto legacy = lv::tokens<legacy_json, legacy_settings>;
static_assert(same_color(*legacy.primitive.color.blue_500, *dark.primitive.color.blue_500));
static_assert(same_color(*legacy.primitive.color.paper, *dark.primitive.color.paper));
static_assert(same_color(*legacy.primitive.color.black, *dark.primitive.color.black));
static_assert(legacy.primitive.space._1 == dark.primitive.space._1 && legacy.primitive.space._6 == dark.primitive.space._6);
static_assert(legacy.primitive.duration.fast == dark.primitive.duration.fast);
static_assert(legacy.card.shadow->blur == dark.card.shadow->blur && same_color(legacy.card.shadow->color, dark.card.shadow->color));
static_assert(legacy.card.border->width == dark.card.border->width && same_color(legacy.card.border->color, dark.card.border->color));
static_assert(legacy.text.title->font_size == dark.text.title->font_size);
static_assert(legacy.text.title->letter_spacing == dark.text.title->letter_spacing);
static_assert(legacy.motion.enter->delay == dark.motion.enter->delay && legacy.card.fill->count == dark.card.fill->count);

// A color object that has components only, or an alpha: the standard's way to write them.
constexpr char object_colors_json[] = R"({"c": {"$type": "color",
  "components": {"$value": {"colorSpace": "srgb", "components": [0.2, 0.4, 1]}},
  "alpha": {"$value": {"colorSpace": "srgb", "components": [0.2, 0.4, 1], "alpha": 0.502, "hex": "#3366ff"}},
  "opaque": {"$value": {"colorSpace": "srgb", "components": [0, 0, 0], "alpha": 1}}}})";
constexpr auto object_colors = lv::tokens<object_colors_json>;
static_assert(std::is_same_v<decltype(object_colors.c.components), lv::token<lv_color_t>>);
static_assert(is_rgb(*object_colors.c.components, 51, 102, 255));
static_assert(std::is_same_v<decltype(object_colors.c.alpha), lv::token<lv::rgba_color>>);
static_assert(is_rgb(object_colors.c.alpha->color, 51, 102, 255) && object_colors.c.alpha->alpha == 128);
static_assert(object_colors.c.opaque->alpha == 255);



// One value of each simple type, in each way it can be written.
template <class T>
consteval T read_as(std::string_view text) {
  const auto v = *lv::json::value::parse(text);
  return lv::detail::read_token(lv::detail::read_as<T>{}, v, v);
}
static_assert(read_as<lv::length>(R"("50%")") == lv::length{50 * lv::pct});
static_assert(read_as<lv::length>(R"({"value": 25, "unit": "%"})") == lv::length{25 * lv::pct});
static_assert(read_as<lv::length>(R"("1.5rem")") == lv::length{24 * px});
static_assert(read_as<lv::length>(R"("8px")") == lv::length{8 * px} && read_as<lv::length>("8") == lv::length{8 * px});
static_assert(read_as<lv::duration>(R"("2s")") == lv::duration{2000 * ms});
static_assert(read_as<lv::duration>(R"("150ms")") == lv::duration{150 * ms});
static_assert(read_as<lv::duration>("40") == lv::duration{40 * ms});
static_assert(read_as<std::int32_t>(R"("normal")") == 400 && read_as<std::int32_t>(R"("bold")") == 700);
static_assert(read_as<std::int32_t>("600") == 600);

static_assert(read_as<lv::bezier>("[0.1, 0.2, 0.3, 0.4]") == lv::bezier{0.1, 0.2, 0.3, 0.4});
// A gradient takes as many stops as it has room for.
consteval std::size_t gradient_stops(std::size_t n) {
  std::string doc = "[";
  for (std::size_t i = 0; i < n; ++i) doc += std::string(i ? "," : "") + R"({"color": "#102030", "position": 0.5})";
  return read_as<lv::gradient_token>(doc + "]").count;
}
static_assert(gradient_stops(8) == 8 && gradient_stops(1) == 1 && gradient_stops(0) == 0);

// Member names: letters, digits and the underscore, at each end of each range.
static_assert(lv::detail::is_name_char('a') && lv::detail::is_name_char('z'));
static_assert(lv::detail::is_name_char('A') && lv::detail::is_name_char('Z'));
static_assert(lv::detail::is_name_char('0') && lv::detail::is_name_char('9') && lv::detail::is_name_char('_'));
static_assert(!lv::detail::is_name_char('`') && !lv::detail::is_name_char('{') && !lv::detail::is_name_char('~'));
static_assert(!lv::detail::is_name_char('@') && !lv::detail::is_name_char('[') && !lv::detail::is_name_char('-'));
static_assert(!lv::detail::is_name_char('/') && !lv::detail::is_name_char(':'));

// Aliases. A chain of `links` aliases that ends in a number; eight links are followed.
consteval double through_aliases(int links) {  // at most 9, so that a name is one digit
  const auto digit = [](int i) { return std::string(1, static_cast<char>('0' + i)); };
  std::string doc = "{\"g\": {";
  for (int i = 0; i < links; ++i)
    doc += "\"t" + digit(i) + "\": {\"$value\": \"{g.t" + digit(i + 1) + "}\"}, ";
  doc += "\"t" + digit(links) + "\": {\"$type\": \"number\", \"$value\": 7}}}";
  const auto root = *lv::json::value::parse(doc);
  return lv::detail::follow_alias(root, "{g.t0}", 0).value.number();
}
static_assert(through_aliases(0) == 7 && through_aliases(1) == 7 && through_aliases(8) == 7);

// A token's own $type wins over the type of the token it points at.
consteval lv::detail::token_type type_of(std::string_view name) {
  const auto root = *lv::json::value::parse(R"({"g": {
      "color": {"$type": "color", "$value": "#102030"},
      "typed": {"$type": "string", "$value": "{g.color}"},
      "plain": {"$value": "{g.color}"}}})");
  return lv::detail::resolve_token(root, root["g"][name], lv::detail::token_type::none).type;
}
static_assert(type_of("typed") == lv::detail::token_type::string);
static_assert(type_of("plain") == lv::detail::token_type::color);

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
  dc.set({.style = lv::style({.radius = lv::raw(3 * px)})});
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
