// A screen of more than a hundred widgets, written as an application writes one:
// small functions that each return a part, nested a few levels deep.
//
// It checks that such a tree mounts whole, and it guards its own compile time.
// CMake compiles this file with a fixed -fconstexpr-ops-limit (tests/CMakeLists.txt):
// the number of steps the compiler may take in one constant evaluation. If a node
// becomes expensive for the compiler again, this file stops compiling.
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constinit lv::state taps{lv::counter{}};
void tapped() { ++taps; }

// A button with its text: 2 widgets.
constexpr auto action(const char* text) {
  return lv::button({.on_clicked = &tapped,
                     .style = {.bg_color = tok.color.primary, .bg_opa = tok.opacity.opaque,
                               .radius = tok.card.radius},
                     .style_pressed = {.bg_color = tok.color.outline}})[
      lv::label({.style = {.text_color = tok.color.text}, .text = text})];
}

// A number with its name: 3 widgets.
constexpr auto reading(const char* name) {
  return lv::row[lv::label({.style = {.text_color = tok.color.text}, .text = name}),
                 lv::label({.text = lv::bind(taps, "{}")})];
}

// A card with a title, two readings and two actions: 12 widgets.
constexpr auto card(const char* title) {
  return lv::box({.style = {.bg_color = tok.color.card, .bg_opa = tok.opacity.opaque,
                            .pad_all = tok.card.padding, .radius = tok.card.radius}})[
      lv::label({.text = title}), reading("now"), reading("total"), action("-"), action("+")];
}

// A page of three cards: 37 widgets.
constexpr auto page(const char* id) {
  return lv::column({.id = id, .size = {100 * pct, 100 * pct}})[
      card("Speed"), card("Power"), card("Range")];
}

// Three pages in a group: 112 widgets.
constexpr auto ui = lv::group({.id = "pages"})[page("first"), page("second"), page("third")];

static_assert(lv::detail::id_count<ui> == 4);

// A node keeps a style only where one is written.
static_assert(action("+").p.style.has && action("+").p.style_pressed.has);
static_assert(!action("+").p.style_checked.has && !reading("now").p.style.has);
static_assert(action("+").p.style_pressed.value.bg_color.set &&
              !action("+").p.style_pressed.value.bg_opa.set);

// lv::group fills its parent and starts without the theme's styles; lv::row and
// lv::column lay their children out. What the node itself says comes first.
static_assert(ui.how == lv::detail::start::group && !ui.p.size.set);
static_assert(lv::group({.size = {10 * px, 20 * px}}).p.size.v.v0 == 10 &&
              lv::group({.size = {10 * px, 20 * px}}).p.size.v.v1 == 20);
static_assert(lv::row().how == lv::detail::start::row && lv::column().how == lv::detail::start::column &&
              !lv::row().p.flex_flow.set);
static_assert(lv::column({.flex_flow = LV_FLEX_FLOW_ROW_WRAP}).p.flex_flow.v.v0 ==
              LV_FLEX_FLOW_ROW_WRAP);

// A node made while the program runs keeps what is set, as one made at compile time.
void keeps_what_is_set() {
  volatile int unknown = 120;  // a width the compiler cannot work out
  const int width = unknown;
  const auto made = lv::button({.size = {width * px, 40 * px},
                                .style_pressed = {.bg_color = tok.color.outline}})[
      lv::label({.text = "made at run time"})];
  check(made.p.size.set && made.p.size.v.v0 == 120 && made.p.size.v.v1 == 40);
  check(!made.p.x.set && !made.p.on_clicked.set);
  check(!made.p.style.has && made.p.style_pressed.has);
  check(made.p.style_pressed.value.bg_color.set && !made.p.style_pressed.value.bg_opa.set);
  check(made.kids.k0.p.text.set && std::strcmp(made.kids.k0.p.text.v.v0, "made at run time") == 0);
}

// The widget `o` and everything under it.
std::uint32_t count_widgets(lv_obj_t* o) {
  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(o); ++i)
    n += count_widgets(lv_obj_get_child(o, static_cast<std::int32_t>(i)));
  return n;
}

// Child number `first` of `o`, then child number `rest` of that, and so on.
template <std::same_as<int>... Rest>
lv_obj_t* child(lv_obj_t* o, int first, Rest... rest) {
  lv_obj_t* kid = lv_obj_get_child(o, first);
  if constexpr (sizeof...(Rest) == 0)
    return kid;
  else
    return child(kid, rest...);
}

bool same_color(lv_color_t a, lv_color_t b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

void mounts_every_widget(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  if (!root) return;

  check(count_widgets(root->root().raw()) == 112);
  lv_obj_update_layout(root->root().raw());
  check(lv_obj_get_width(root->root().raw()) == 320 && lv_obj_get_height(root->root().raw()) == 240);
  check(root->get<"pages">().raw() == root->root().raw());
  check(lv_obj_get_child_count(root->get<"second">().raw()) == 3);
  check(root->get<"third">().raw() == child(root->root().raw(), 2));

  // The last card of the last page is as complete as the first one.
  lv_obj_t* last_card = child(root->root().raw(), 2, 2);
  check(lv_obj_get_child_count(last_card) == 5);
  check(std::strcmp(lv_label_get_text(child(last_card, 0)), "Range") == 0);
  check(std::strcmp(lv_label_get_text(child(last_card, 4, 0)), "+") == 0);
  check(same_color(lv_obj_get_style_bg_color(last_card, LV_PART_MAIN), *tok.color.card));

  // A style for one state is applied for that state only.
  lv_obj_t* plus = child(last_card, 4);
  check(same_color(lv_obj_get_style_bg_color(plus, LV_PART_MAIN), *tok.color.primary));
  lv_obj_add_state(plus, LV_STATE_PRESSED);
  check(same_color(lv_obj_get_style_bg_color(plus, LV_PART_MAIN), *tok.color.outline));
  lv_obj_remove_state(plus, LV_STATE_PRESSED);

  // Every handler and every binding is wired: one click is seen by all 18 readings.
  lv_obj_send_event(plus, LV_EVENT_CLICKED, nullptr);
  check(taps.get().value == 1);
  check(std::strcmp(lv_label_get_text(child(root->root().raw(), 0, 0, 1, 1)), "1") == 0);
  check(std::strcmp(lv_label_get_text(child(last_card, 2, 1)), "1") == 0);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  keeps_what_is_set();
  mounts_every_widget(lv::screen_active());
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
