// lv::look: a group of the project's tokens as a style, by name.

#include <lv/font.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/repeat.hpp>
#include <lv/theme.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr char other_json[] = {
#embed "look_other.json"
    , 0};
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto tok = lv::tokens<tokens_json>;
constexpr auto other = lv::tokens<other_json>;

struct other_design {};

}  // namespace

template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{
    .tokens = tok, .type = lv::typeface({{"Barlow", 500, barlow}})};
template <>
inline constexpr lv::design_of<decltype(other)> lv::design<other_design>{.tokens = other};

namespace {

// ---- what the compiler knows, without a display ----

// A look finds the style property of an entry by halving the members of lv::style, which needs
// them in alphabetical order. A template, because GCC 16.2 warns that `i` shadows itself in a plain function.
template <class D = lv::override>
consteval bool every_style_property_is_found() {
  const auto members = lv::detail::style_members<std::void_t<D>>;
  for (std::size_t i = 0; i < members.size(); ++i) {
    if (lv::detail::style_property<D>(std::meta::identifier_of(members[i])) != members[i]) return false;
    if (i > 0 && !(std::meta::identifier_of(members[i - 1]) < std::meta::identifier_of(members[i]))) return false;
  }
  return lv::detail::style_property<D>("no_such_property") == std::meta::info{};
}
static_assert(every_style_property_is_found());

static_assert(lv::style_value<decltype(lv::look<"tile">)>);
static_assert(lv::look<"tile">.radius.set && lv::look<"tile">.width.set && lv::look<"tile">.text_font.set);
static_assert(!lv::look<"tile">.bg_color.set, "bg_color is in the variants");
// A look that sets bg_color does not set bg_opa by itself.
static_assert(lv::look<"tile-print">.bg_color.set && !lv::look<"tile-print">.bg_opa.set);
static_assert(lv::look<"status">.layers.count == 1 && lv::look<"tile">.layers.count == 0);
static_assert(lv::look<"gauge">.layers.count == 3);  // indicator, knob, knob + pressed
// A shorthand is held as its long forms: a constant style has no value under its own name.
static_assert(!lv::look<"padded">.pad_all.set && lv::look<"padded">.pad_top.set && lv::look<"padded">.pad_column.set);
// Enumerators and percentages come from "$extensions"; a color's alpha sets the opacity beside it.
static_assert(lv::look<"flex-ext">.flex_flow.set && lv::look<"flex-ext">.layout.set && lv::look<"flex-ext">.layers.count == 1);
static_assert(lv::look<"pct">.width.set && lv::look<"pct">.height.set);
static_assert(lv::look<"alpha">.bg_opa.set && lv::look<"alpha">.bg_color.set);
static_assert(!lv::look<"alpha">.text_opa.set, "an opaque color sets no opacity");
static_assert(lv::look<"alpha-explicit">.bg_opa.set);
// `alt` sorts before `any`, and is a state like the others; the opacity of a long name (`border_color`) is its own.
static_assert(lv::look<"alt-layer">.layers.count == 1);
static_assert(lv::look<"alpha-border">.border_opa.set && lv::look<"alpha-border">.border_color.set);

// A group named for the default state (the token reader names its member default_) adds no selector:
// its entries are the look's own, and beat the entries beside it.
static_assert(lv::look<"lit">.layers.count == 0 && lv::look<"lit">.radius.set);
static_assert(lv::look<"main-wide">.layers.count == 0 && lv::look<"main-wide">.width.set);

// Spaces between looks, any number.
static_assert(lv::look<"tile tile-print">.bg_color.set && lv::look<"tile-print"  " tile">.radius.set);

constexpr auto tile_ui = lv::button({.id = "tile", .style = lv::look<"tile"> | lv::look<"tile-print">})[
    lv::group({.id = "inner", .style = lv::look<"tile-inner">})];

constexpr auto status_ui = lv::label({.id = "status", .checked = true, .text = "PRINTING", .style = lv::look<"status">});
constexpr auto later_wins_ui = lv::label({.id = "status", .checked = true, .text = "X", .style = lv::look<"status status-loud">});
constexpr auto padded_ui = lv::box({.id = "padded", .checked = true, .style = lv::look<"padded">});
constexpr auto gauge_ui = lv::slider({.id = "gauge", .style = lv::look<"gauge">});
constexpr auto card_ui = lv::box({.id = "card", .style = lv::look<"card">});
constexpr auto inherit_ui = lv::group({.id = "parent", .style = lv::look<"plain">})[lv::label({.id = "child", .text = "text"})];

// A look merged with lv::on and with a style: the later one wins, the layers are kept.
constexpr auto combined_ui = lv::label({.id = "combined", .checked = true,
                                        .style = lv::look<"status"> | lv::on("pressed", {.text_color = tok.color.ink}) |
                                                 lv::style({.radius = tok.flat.pad_all})});

// Two looks that ask for the same typeface file and size share one drawn font.
constexpr auto twin_ui = lv::box({.id = "twin", .style = lv::look<"twin">});
constexpr auto tile_alone_ui = lv::box({.id = "alone", .style = lv::look<"tile">});

constexpr auto pct_ui = lv::box({.id = "pct", .style = lv::look<"pct">});
constexpr auto alpha_ui = lv::box({.id = "alpha", .style = lv::look<"alpha">});
constexpr auto alpha_explicit_ui = lv::box({.id = "explicit", .style = lv::look<"alpha-explicit">});
constexpr auto pad_order_ui = lv::box({.id = "order", .style = lv::look<"pad-order">});
constexpr auto pad_narrow_ui = lv::box({.id = "narrow", .style = lv::look<"pad-narrow">});
constexpr auto pad_narrow_after_ui = lv::box({.id = "after", .style = lv::look<"pad-narrow-after">});
constexpr auto main_then_wide_ui = lv::box({.id = "main_then_wide", .style = lv::look<"main-wide wide">});
// A node's own style, lv::on and a theme follow the rule of a look: a long form beats a shorthand, and a narrower
// shorthand (pad_hor) beats a wider one (pad_all).
constexpr auto shorthands_style = lv::style({.pad_all = tok.flat.pad_all, .pad_hor = tok.tile.radius, .pad_right = tok.tile.width});
constexpr auto on_shorthands_ui = lv::box({.id = "on_shorthands", .checked = true, .style = lv::on("checked", shorthands_style)});
constexpr auto own_shorthands_ui = lv::box({.id = "own_shorthands", .style = shorthands_style});
constexpr auto shorthand_theme = lv::theme({.obj = {.style = shorthands_style}});
constexpr auto lit_ui = lv::box({.id = "lit", .style = lv::look<"lit">});
// The default group is written first, and still beats the entry beside it.
constexpr auto lit_first_ui = lv::box({.id = "lit_first", .style = lv::look<"lit-first">});
constexpr auto pct99_ui = lv::box({.id = "pct99", .style = lv::look<"pct99">});
constexpr auto alpha_border_ui = lv::box({.id = "alpha_border", .style = lv::look<"alpha-border">});
constexpr auto ext_layer_ui = lv::label({.id = "ext", .checked = true, .text = "x", .style = lv::look<"flex-ext">});

// Defaults of a factory give way to a look that says it.
constexpr auto fills_ui = lv::group({.id = "fills"});
constexpr auto sized_ui = lv::group({.id = "sized", .style = lv::look<"tile">});
constexpr auto wide_ui = lv::group({.id = "wide", .style = lv::look<"wide">});
constexpr auto own_ui = lv::group({.id = "own", .size = {30 * px, 20 * px}, .style = lv::look<"tile">});
constexpr auto own_width_ui = lv::group({.id = "own_width", .width = 30 * px, .style = lv::look<"tall">});
constexpr auto own_height_ui = lv::group({.id = "own_height", .height = 20 * px, .style = lv::look<"wide">});
constexpr auto checked_wide_ui = lv::group({.id = "checked_wide", .checked = true, .style = lv::look<"checked-wide">});
constexpr auto width_only_ui = lv::group({.id = "width_only", .width = 30 * px});
constexpr auto flow_ui = lv::row({.id = "flow", .style = lv::look<"tile">});
constexpr auto row_ui = lv::row({.id = "row"});
constexpr auto main_wide_ui = lv::group({.id = "main_wide", .style = lv::look<"main-wide">});
constexpr auto main_flow_ui = lv::row({.id = "main_flow", .style = lv::look<"main-flow">});
constexpr auto flow_only_ui = lv::row({.id = "flow_only", .style = lv::look<"flow-only">});
constexpr auto flow_no_layout_ui = lv::column({.id = "flow_no_layout", .style = lv::look<"flow-no-layout">});

// The look named for an enumerator, in a part that lv::for_each repeats.
enum class subpage { none, print, scan };
constexpr auto tile = []<subpage Page> {
  return lv::button({.style = lv::look<"tile"> | lv::look_of<"tile", Page>});
};
constexpr auto tiles_ui = lv::group({.id = "tiles"})[lv::for_each<subpage, lv::but<subpage::none>>(tile)];
static_assert(lv::look_of<"tile", subpage::print>.bg_color.set && !lv::look_of<"tile", subpage::print>.radius.set);

// A part library takes the design as a parameter.
template <class Design = lv::override>
constexpr auto chip = lv::box({.style = lv::look<"chip", Design>});
constexpr auto chip_ui = lv::group[chip<other_design>];

constexpr auto themed = lv::theme({.obj = {.style = lv::look<"padded">}});

lv::application* the_app = nullptr;

unsigned rgb(lv_color_t c) { return lv_color_to_u32(c) & 0xffffffu; }

constinit lv::frame<320, 240> frame;
constinit lv::frame<64, 48> small_frame;

// LVGL keeps one entry in a widget's list of styles for each style it adds.
constexpr std::size_t style_entry = 16;

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

void a_look_sets_what_its_entries_name() {
  auto built = lv::mount<tile_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* b = built->get<"tile">().raw();
  lv_obj_update_layout(b);
  check(lv_obj_get_style_radius(b, LV_PART_MAIN) == 24);
  check(lv_obj_get_width(b) == 162 && lv_obj_get_height(b) == 192);
  check(rgb(lv_obj_get_style_bg_color(b, LV_PART_MAIN)) == 0x2b7fff);
  check(rgb(lv_obj_get_style_bg_grad_color(b, LV_PART_MAIN)) == 0x155dfc);
  check(lv_obj_get_style_bg_opa(b, LV_PART_MAIN) == LV_OPA_COVER);
  check(lv_obj_get_style_bg_grad_dir(b, LV_PART_MAIN) == LV_GRAD_DIR_VER);
  check(lv_obj_get_style_text_align(b, LV_PART_MAIN) == LV_TEXT_ALIGN_CENTER);
  check(lv_obj_get_style_pad_row(b, LV_PART_MAIN) == 10);
  check(lv_obj_get_style_flex_flow(b, LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  check(lv_obj_get_style_flex_main_place(b, LV_PART_MAIN) == LV_FLEX_ALIGN_CENTER);
  check(rgb(lv_obj_get_style_text_color(b, LV_PART_MAIN)) == 0xffffff);
  const lv_font_t* font = lv_obj_get_style_text_font(b, LV_PART_MAIN);
  check(font != lv_font_get_default() && font->line_height > 18);
  lv_obj_t* inner = built->get<"inner">().raw();
  check(lv_obj_get_style_radius(inner, LV_PART_MAIN) == 16);
  check(lv_obj_get_style_bg_opa(inner, LV_PART_MAIN) == LV_OPA_20);
  check(lv_obj_get_width(inner) == 64);
}

void looks_with_the_same_font_share_it() {
  auto twin = lv::mount<twin_ui>(lv::screen_active());
  auto alone = lv::mount<tile_alone_ui>(lv::screen_active());
  check(twin.has_value() && alone.has_value());
  if (!twin || !alone) return;
  const lv_font_t* first = lv_obj_get_style_text_font(twin->root().raw(), LV_PART_MAIN);
  check(first == lv_obj_get_style_text_font(alone->root().raw(), LV_PART_MAIN));
  check(first != lv_font_get_default());
}

void a_percentage_is_read_from_the_extensions() {
  auto built = lv::mount<pct_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* b = built->get<"pct">().raw();
  check(lv_obj_get_style_width(b, LV_PART_MAIN) == LV_PCT(50));  // the $value of 100px is for design tools
  check(lv_obj_get_style_height(b, LV_PART_MAIN) == LV_PCT(25));
}

void a_color_with_alpha_sets_its_opacity() {
  auto built = lv::mount<alpha_ui>(lv::screen_active());
  auto explicit_opa = lv::mount<alpha_explicit_ui>(lv::screen_active());
  check(built.has_value() && explicit_opa.has_value());
  if (!built || !explicit_opa) return;
  lv_obj_t* b = built->get<"alpha">().raw();
  check(rgb(lv_obj_get_style_bg_color(b, LV_PART_MAIN)) == 0x2b7fff);
  check(lv_obj_get_style_bg_opa(b, LV_PART_MAIN) == 0x80);
  // The group says bg_opa itself: it wins over the alpha of the color.
  check(lv_obj_get_style_bg_opa(explicit_opa->get<"explicit">().raw(), LV_PART_MAIN) == LV_OPA_COVER);
}

void the_extensions_of_a_layer_are_its_own() {
  auto built = lv::mount<ext_layer_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* l = built->get<"ext">().raw();
  check(lv_obj_get_style_flex_flow(l, LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
  check(lv_obj_get_style_text_align(l, LV_PART_MAIN) == LV_TEXT_ALIGN_RIGHT);  // checked
  lv_obj_remove_state(l, LV_STATE_CHECKED);
  check(lv_obj_get_style_text_align(l, LV_PART_MAIN) != LV_TEXT_ALIGN_RIGHT);
}

void a_state_group_is_a_layer() {
  auto built = lv::mount<status_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* s = built->get<"status">().raw();
  check(rgb(lv_obj_get_style_text_color(s, LV_PART_MAIN)) == 0x155dfc);
  lv_obj_remove_state(s, LV_STATE_CHECKED);
  check(rgb(lv_obj_get_style_text_color(s, LV_PART_MAIN)) == 0x62748e);
}

void a_default_state_group_is_a_layer() {
  auto built = lv::mount<lit_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  check(lv_obj_get_style_radius(built->get<"lit">().raw(), LV_PART_MAIN) == 9);
}

void the_default_group_beats_the_entry_beside_it_wherever_it_is_written() {
  auto built = lv::mount<lit_first_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  check(lv_obj_get_style_radius(built->get<"lit_first">().raw(), LV_PART_MAIN) == 9);
}

void a_percentage_of_nines_and_a_long_color_name() {
  auto nines = lv::mount<pct99_ui>(lv::screen_active());
  auto border = lv::mount<alpha_border_ui>(lv::screen_active());
  check(nines.has_value() && border.has_value());
  if (!nines || !border) return;
  check(lv_obj_get_style_width(nines->get<"pct99">().raw(), LV_PART_MAIN) == LV_PCT(99));
  // border_color has twelve characters: its opacity is border_opa, set from the alpha of the color.
  check(lv_obj_get_style_border_opa(border->get<"alpha_border">().raw(), LV_PART_MAIN) == 0x80);
}

void a_later_look_wins_in_a_layer_too() {
  auto built = lv::mount<later_wins_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* s = built->get<"status">().raw();
  check(rgb(lv_obj_get_style_text_color(s, LV_PART_MAIN)) == 0xff2056);
}

void parts_and_states_make_layers() {
  auto built = lv::mount<gauge_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* g = built->get<"gauge">().raw();
  check(rgb(lv_obj_get_style_bg_color(g, LV_PART_MAIN)) == 0x62748e);
  check(rgb(lv_obj_get_style_bg_color(g, LV_PART_INDICATOR)) == 0x155dfc);
  check(rgb(lv_obj_get_style_bg_color(g, LV_PART_KNOB)) == 0xffffff);
  lv_obj_add_state(g, LV_STATE_PRESSED);
  const auto pressed_knob = static_cast<lv_part_t>(static_cast<unsigned>(LV_PART_KNOB) | LV_STATE_PRESSED);
  check(rgb(lv_obj_get_style_prop(g, pressed_knob, LV_STYLE_BG_COLOR).color) == 0xff2056);
}

void composite_tokens_are_merged() {
  auto built = lv::mount<card_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* c = built->get<"card">().raw();
  check(lv_obj_get_style_shadow_width(c, LV_PART_MAIN) == 12 && lv_obj_get_style_shadow_offset_y(c, LV_PART_MAIN) == 4);
  check(lv_obj_get_style_shadow_spread(c, LV_PART_MAIN) == 2);
  check(lv_obj_get_style_border_width(c, LV_PART_MAIN) == 3);
  check(rgb(lv_obj_get_style_border_color(c, LV_PART_MAIN)) == 0x1d293d);
  check(lv_obj_get_style_anim_duration(c, LV_PART_MAIN) == 150);
  check(lv_obj_get_style_text_letter_space(c, LV_PART_MAIN) == 1);
  check(lv_obj_get_style_text_font(c, LV_PART_MAIN) != lv_font_get_default());
}

void a_shorthand_does_not_beat_a_layer() {
  auto built = lv::mount<padded_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* p = built->get<"padded">().raw();
  check(lv_obj_get_style_pad_top(p, LV_PART_MAIN) == 3);  // checked
  check(lv_obj_get_style_pad_bottom(p, LV_PART_MAIN) == 7 && lv_obj_get_style_pad_left(p, LV_PART_MAIN) == 7);
  check(lv_obj_get_style_pad_row(p, LV_PART_MAIN) == 5 && lv_obj_get_style_pad_column(p, LV_PART_MAIN) == 5);
  lv_obj_remove_state(p, LV_STATE_CHECKED);
  check(lv_obj_get_style_pad_top(p, LV_PART_MAIN) == 7);
}

// A JSON object has no order: an entry the group names itself beats a shorthand that covers it,
// and a narrower shorthand (pad_hor) beats a wider one (pad_all), written in either order.
void a_long_form_beats_a_shorthand_in_any_order() {
  auto order = lv::mount<pad_order_ui>(lv::screen_active());
  auto narrow = lv::mount<pad_narrow_ui>(lv::screen_active());
  auto after = lv::mount<pad_narrow_after_ui>(lv::screen_active());
  check(order.has_value() && narrow.has_value() && after.has_value());
  if (!order || !narrow || !after) return;
  lv_obj_t* o = order->root().raw();
  check(lv_obj_get_style_pad_top(o, LV_PART_MAIN) == 3 && lv_obj_get_style_pad_bottom(o, LV_PART_MAIN) == 7);
  for (lv_obj_t* n : {narrow->root().raw(), after->root().raw()}) {
    check(lv_obj_get_style_pad_left(n, LV_PART_MAIN) == 2 && lv_obj_get_style_pad_right(n, LV_PART_MAIN) == 2);
    check(lv_obj_get_style_pad_top(n, LV_PART_MAIN) == 7 && lv_obj_get_style_pad_bottom(n, LV_PART_MAIN) == 7);
  }
}

// A main group is the look itself, so a later look in the list beats it as it beats the entries.
void a_later_look_beats_a_main_group() {
  auto built = lv::mount<main_then_wide_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_update_layout(built->root().raw());
  check(lv_obj_get_width(built->root().raw()) == 100);
}

void check_one_rule(lv_obj_t* o) {
  check(lv_obj_get_style_pad_top(o, LV_PART_MAIN) == 7 && lv_obj_get_style_pad_bottom(o, LV_PART_MAIN) == 7);
  check(lv_obj_get_style_pad_left(o, LV_PART_MAIN) == 24);
  check(lv_obj_get_style_pad_right(o, LV_PART_MAIN) == 162);
}

void a_node_on_and_a_theme_follow_the_one_rule() {
  auto built = lv::mount<on_shorthands_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  check_one_rule(built->root().raw());
  auto own = lv::mount<own_shorthands_ui>(lv::screen_active());
  check(own.has_value());
  if (own) check_one_rule(own->root().raw());
  lv::display display{*the_app, small_frame, shorthand_theme};
  check_one_rule(lv_obj_create(display.screen().raw()));
}

void a_label_inherits_the_text_of_its_parents_look() {
  auto built = lv::mount<inherit_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* child = built->get<"child">().raw();
  check(rgb(lv_obj_get_style_text_color(child, LV_PART_MAIN)) == 0xff2056);
  check(lv_obj_get_style_text_font(child, LV_PART_MAIN) == lv_obj_get_style_text_font(built->get<"parent">().raw(), LV_PART_MAIN));
  lv_obj_t* loose = lv_label_create(lv_screen_active());  // a bare label outside it keeps LVGL's
  check(rgb(lv_obj_get_style_text_color(loose, LV_PART_MAIN)) != 0xff2056);
}

void a_look_merges_with_on_and_style() {
  auto built = lv::mount<combined_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* l = built->get<"combined">().raw();
  check(rgb(lv_obj_get_style_text_color(l, LV_PART_MAIN)) == 0x155dfc);  // the look's checked layer
  check(lv_obj_get_style_radius(l, LV_PART_MAIN) == 7);
  lv_obj_add_state(l, LV_STATE_PRESSED);
  check(rgb(lv_obj_get_style_text_color(l, LV_PART_MAIN)) == 0x1d293d);
}

// A group fills its parent, unless a look sets its width or its height.
void a_groups_default_gives_way_to_a_look() {
  const auto fills = lv::mount<fills_ui>(lv::screen_active());
  const auto sized = lv::mount<sized_ui>(lv::screen_active());
  const auto wide = lv::mount<wide_ui>(lv::screen_active());
  const auto own = lv::mount<own_ui>(lv::screen_active());
  check(fills.has_value());
  check(sized.has_value());
  check(wide.has_value());
  check(own.has_value());
  if (!fills || !sized || !wide || !own) return;
  lv_obj_update_layout(lv_screen_active());
  const auto screen_w = lv_obj_get_width(lv_screen_active());
  const auto screen_h = lv_obj_get_height(lv_screen_active());
  check(lv_obj_get_width(fills->root().raw()) == screen_w);  // no look: the group fills
  check(lv_obj_get_width(sized->root().raw()) == 162);
  check(lv_obj_get_height(sized->root().raw()) == 192);
  // A look with a width only: the height still fills.
  check(lv_obj_get_width(wide->root().raw()) == 100);
  check(lv_obj_get_height(wide->root().raw()) == screen_h);
  // The node's own .size beats the look.
  check(lv_obj_get_width(own->root().raw()) == 30);
  check(lv_obj_get_height(own->root().raw()) == 20);
}

// The node's own .width or .height beats both the look and the default, whichever side the look sets.
void a_groups_own_side_beats_its_default() {
  const auto own_width = lv::mount<own_width_ui>(lv::screen_active());
  const auto own_height = lv::mount<own_height_ui>(lv::screen_active());
  const auto width_only = lv::mount<width_only_ui>(lv::screen_active());
  check(own_width.has_value() && own_height.has_value() && width_only.has_value());
  if (!own_width || !own_height || !width_only) return;
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(own_width->root().raw()) == 30 && lv_obj_get_height(own_width->root().raw()) == 50);
  check(lv_obj_get_width(own_height->root().raw()) == 100 && lv_obj_get_height(own_height->root().raw()) == 20);
  check(lv_obj_get_width(width_only->root().raw()) == 30);
  check(lv_obj_get_height(width_only->root().raw()) == lv_obj_get_height(lv_screen_active()));
}

// A width in a state layer beats the group's default in that state: LVGL ranks a style for a state
// above one for the default state, a local one too. Out of the state, the default of 100% is back.
void a_state_layer_beats_the_groups_default() {
  const auto built = lv::mount<checked_wide_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* g = built->root().raw();
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(g) == 40);
  lv_obj_remove_state(g, LV_STATE_CHECKED);
  check(lv_obj_get_style_width(g, LV_PART_MAIN) == lv_pct(100));
}

// lv::row is a flex row, unless the look says the flow.
void a_rows_flow_gives_way_to_a_look() {
  const auto flow = lv::mount<flow_ui>(lv::screen_active());
  const auto plain_row = lv::mount<row_ui>(lv::screen_active());
  check(flow.has_value());
  check(plain_row.has_value());
  if (!flow || !plain_row) return;
  check(lv_obj_get_style_flex_flow(flow->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  check(lv_obj_get_style_flex_flow(plain_row->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
}

// A group named main or default adds no selector: it is the look itself, so a factory's default
// gives way to what it sets, as to the look's own entries.
void a_main_group_counts_as_the_look() {
  const auto wide = lv::mount<main_wide_ui>(lv::screen_active());
  const auto flow = lv::mount<main_flow_ui>(lv::screen_active());
  check(wide.has_value() && flow.has_value());
  if (!wide || !flow) return;
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(wide->root().raw()) == 40);
  check(lv_obj_get_height(wide->root().raw()) == lv_obj_get_height(lv_screen_active()));
  check(lv_obj_get_style_flex_flow(flow->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
}

std::int32_t layout_of(lv_obj_t* o) { return lv_obj_get_style_prop(o, LV_PART_MAIN, LV_STYLE_LAYOUT).num; }

// A look that sets the flow only still gets a flex layout, as the factory's flow would have: LVGL's
// setter of the flow sets the layout too, a style's flow does not. A look's own layout is kept.
void a_rows_look_flow_keeps_the_flex_layout() {
  const auto flow_only = lv::mount<flow_only_ui>(lv::screen_active());
  const auto no_layout = lv::mount<flow_no_layout_ui>(lv::screen_active());
  check(flow_only.has_value() && no_layout.has_value());
  if (!flow_only || !no_layout) return;
  check(lv_obj_get_style_flex_flow(flow_only->root().raw(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  check(layout_of(flow_only->root().raw()) == LV_LAYOUT_FLEX);
  check(layout_of(no_layout->root().raw()) == LV_LAYOUT_NONE);
}

void a_variant_for_each_enumerator() {
  auto built = lv::mount<tiles_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* group = built->root().raw();
  check(lv_obj_get_child_count(group) == 2);
  check(rgb(lv_obj_get_style_bg_color(lv_obj_get_child(group, 0), LV_PART_MAIN)) == 0x2b7fff);
  check(rgb(lv_obj_get_style_bg_color(lv_obj_get_child(group, 1), LV_PART_MAIN)) == 0x00bc7d);
}

void a_part_takes_the_design() {
  auto built = lv::mount<chip_ui>(lv::screen_active());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* c = lv_obj_get_child(built->root().raw(), 0);
  check(rgb(lv_obj_get_style_bg_color(c, LV_PART_MAIN)) == 0x009688 && lv_obj_get_style_radius(c, LV_PART_MAIN) == 4);
}

void a_theme_takes_a_look() {
  lv::application& app = *the_app;
  lv::display display{app, small_frame, themed};
  lv_obj_t* box = lv_obj_create(display.screen().raw());
  check(lv_obj_get_style_pad_top(box, LV_PART_MAIN) == 7 && lv_obj_get_style_pad_row(box, LV_PART_MAIN) == 5);
}

// A look is a constant: a widget that has one takes no more heap than a plain one, apart from
// LVGL's list of its styles (one entry for the look, one for each layer).
constexpr auto plain_ui = lv::box({});
constexpr auto looked_ui = lv::box({.style = lv::look<"tile"> | lv::look<"tile-print">});
constexpr auto layered_ui = lv::box({.style = lv::look<"gauge">});

std::size_t heap_of_mounted(auto mounted) {
  const auto start = used_bytes();
  auto built = mounted();
  return used_bytes() - start;
}

void a_look_costs_no_heap_beyond_the_style_list() {
  const auto plain = heap_of_mounted([] { return lv::mount<plain_ui>(lv::screen_active()); });
  const auto looked = heap_of_mounted([] { return lv::mount<looked_ui>(lv::screen_active()); });
  const auto layered = heap_of_mounted([] { return lv::mount<layered_ui>(lv::screen_active()); });
  std::printf("heap: a box %zu bytes, with two looks %zu, with a look of three layers %zu\n", plain, looked, layered);
  check(looked <= plain + style_entry && layered <= plain + 4 * style_entry);
}

}  // namespace

int main() {
  lv::application app;
  the_app = &app;
  lv::display display{app, frame};
  a_look_sets_what_its_entries_name();
  looks_with_the_same_font_share_it();
  a_percentage_is_read_from_the_extensions();
  the_default_group_beats_the_entry_beside_it_wherever_it_is_written();
  a_percentage_of_nines_and_a_long_color_name();
  a_color_with_alpha_sets_its_opacity();
  the_extensions_of_a_layer_are_its_own();
  a_state_group_is_a_layer();
  a_default_state_group_is_a_layer();
  a_later_look_wins_in_a_layer_too();
  parts_and_states_make_layers();
  composite_tokens_are_merged();
  a_shorthand_does_not_beat_a_layer();
  a_long_form_beats_a_shorthand_in_any_order();
  a_label_inherits_the_text_of_its_parents_look();
  a_look_merges_with_on_and_style();
  a_groups_default_gives_way_to_a_look();
  a_rows_flow_gives_way_to_a_look();
  a_variant_for_each_enumerator();
  a_part_takes_the_design();
  a_look_costs_no_heap_beyond_the_style_list();
  a_rows_look_flow_keeps_the_flex_layout();
  a_later_look_beats_a_main_group();
  a_node_on_and_a_theme_follow_the_one_rule();
  a_main_group_counts_as_the_look();
  a_state_layer_beats_the_groups_default();
  a_groups_own_side_beats_its_default();  // after it: its widgets move the heap's free blocks
  a_theme_takes_a_look();
  return failures;
}
