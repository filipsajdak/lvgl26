// lv::on: a style for one part in one state, merged into a node's style or a theme's.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>

namespace {

using namespace lv::units;

constexpr lv_color_t red{.blue = 0, .green = 0, .red = 200};
constexpr lv_color_t blue{.blue = 200, .green = 0, .red = 0};
constexpr lv_color_t green{.blue = 0, .green = 200, .red = 0};
constexpr lv_color_t grey{.blue = 90, .green = 90, .red = 90};

constexpr auto base = lv::style({.bg_color = lv::raw(grey)});
constexpr auto knob_pressed = lv::on("knob", "pressed", {.bg_color = lv::raw(red)});
constexpr auto styled_knob = base | knob_pressed;

constexpr lv_style_selector_t pick(lv_part_t part, lv_state_t state) {
  return static_cast<lv_style_selector_t>(part) | static_cast<lv_style_selector_t>(state);
}

using style_type = std::remove_cvref_t<decltype(base)>;

// ---- what lv::on makes, worked out by the compiler -------------------------------

// A layer is a selector and a constant style; a style that holds only layers is set.
static_assert(knob_pressed.layers.count == 1);
static_assert(knob_pressed.layers.first[0].selector == pick(LV_PART_KNOB, LV_STATE_PRESSED));
static_assert(lv::detail::is_set(knob_pressed) && !lv::detail::sets_a_value(knob_pressed));
static_assert(!lv::detail::is_set(style_type{}) && base.layers.count == 0);

// The merge keeps the values of the base and adds the layer.
static_assert(styled_knob.bg_color.set && styled_knob.layers.count == 1);
static_assert(styled_knob.layers.first[0].style == knob_pressed.layers.first[0].style);

// Equal values give one constant style, as the styles of a tree do.
static_assert(lv::on("knob", "pressed", {.bg_color = lv::raw(red)}).layers.first[0].style ==
              knob_pressed.layers.first[0].style);
static_assert(lv::on("indicator", {.bg_color = lv::raw(red)}).layers.first[0].style ==
              knob_pressed.layers.first[0].style);

// One name: a part in the default state, or a state of the main part.
static_assert(lv::on("knob", {.bg_color = lv::raw(red)}).layers.first[0].selector == LV_PART_KNOB);
static_assert(lv::on("pressed", {.bg_color = lv::raw(red)}).layers.first[0].selector == LV_STATE_PRESSED);
static_assert(lv::on("main", {.bg_color = lv::raw(red)}).layers.first[0].selector == LV_PART_MAIN);
static_assert(lv::on("default", {.bg_color = lv::raw(red)}).layers.first[0].selector == LV_STATE_DEFAULT);
static_assert(lv::on("items", "checked", {.bg_color = lv::raw(red)}).layers.first[0].selector ==
              pick(LV_PART_ITEMS, LV_STATE_CHECKED));
static_assert(lv::on("cursor", "focused", {.bg_color = lv::raw(red)}).layers.first[0].selector ==
              pick(LV_PART_CURSOR, LV_STATE_FOCUSED));
static_assert(lv::on("custom_first", "user_4", {.bg_color = lv::raw(red)}).layers.first[0].selector ==
              pick(LV_PART_CUSTOM_FIRST, LV_STATE_USER_4));

// Layers keep their order: the last one written is added last, and wins a tie.
constexpr auto two = base | lv::on("knob", {.bg_color = lv::raw(blue)}) | knob_pressed;
static_assert(two.layers.count == 2 && two.layers.first[0].selector == LV_PART_KNOB &&
              two.layers.first[1].selector == pick(LV_PART_KNOB, LV_STATE_PRESSED));
constexpr auto joined = (base | knob_pressed) | (lv::style({.radius = lv::raw(4 * px)}) | lv::on("checked", {.bg_color = lv::raw(blue)}));
static_assert(joined.layers.count == 2 && joined.layers.first[1].selector == LV_STATE_CHECKED);
static_assert(joined.radius.set && joined.bg_color.set);
// A style merged into a layered one keeps its layers, and tokens keep them too.
static_assert((knob_pressed | base).layers.count == 1);

// A shorthand in a layer is spelled out in the constant style.
constexpr auto padded = lv::on("knob", {.pad_all = lv::raw(5 * px), .pad_left = lv::raw(2 * px)});
constexpr std::size_t count_props(const lv_style_t* s) {
  const auto* p = static_cast<const lv_style_const_prop_t*>(s->values_and_props);
  std::size_t n = 0;
  while (p[n].prop != LV_STYLE_PROP_INV) ++n;
  return n;
}
static_assert(count_props(padded.layers.first[0].style) == 4);

// ---- mounted: the examples this makes possible ------------------------------------

constinit lv::frame<64, 48> frame;

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

lv_color_t bg(lv_obj_t* o, lv_part_t part) { return lv_obj_get_style_bg_color(o, part); }

// slider_styling: the knob while pressed.
constexpr auto slider_ui = lv::box[lv::slider({.id = "slider", .style = styled_knob})];

void slider_knob_pressed(lv::display& display) {
  auto root = lv::mount<slider_ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  lv_obj_t* slider = root->get<"slider">().raw();
  check(same(bg(slider, LV_PART_MAIN), grey));
  check(!same(bg(slider, LV_PART_KNOB), red));
  lv_obj_add_state(slider, LV_STATE_PRESSED);
  check(same(bg(slider, LV_PART_KNOB), red));
  check(same(bg(slider, LV_PART_MAIN), grey));  // only the knob
}

// switch_styling and checkbox_styling: the indicator while checked.
constexpr auto indicator_checked = lv::on("indicator", "checked", {.bg_color = lv::raw(green)});
constexpr auto check_ui = lv::box[lv::toggle({.id = "switch", .style = indicator_checked}),
                                  lv::checkbox({.id = "box", .style = base | indicator_checked})];

void indicator_when_checked(lv::display& display) {
  auto root = lv::mount<check_ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  for (lv_obj_t* o : {root->get<"switch">().raw(), root->get<"box">().raw()}) {
    check(!same(bg(o, LV_PART_INDICATOR), green));
    lv_obj_add_state(o, LV_STATE_CHECKED);
    check(same(bg(o, LV_PART_INDICATOR), green));
    check(!same(bg(o, LV_PART_KNOB), green));
  }
}

// buttonmatrix_styling: the items while checked. textarea_cursor: the cursor while focused.
constexpr auto items_ui = lv::box[
    lv::widget<"buttonmatrix">({.id = "matrix", .style = lv::on("items", "checked", {.bg_color = lv::raw(blue)})}),
    lv::textarea({.id = "text", .style = lv::on("cursor", "focused", {.bg_color = lv::raw(red)})})];

void items_and_cursor(lv::display& display) {
  auto root = lv::mount<items_ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  lv_obj_t* matrix = root->get<"matrix">().raw();
  check(!same(bg(matrix, LV_PART_ITEMS), blue));
  lv_obj_add_state(matrix, LV_STATE_CHECKED);  // LVGL draws a checked button in this state
  check(same(bg(matrix, LV_PART_ITEMS), blue));
  lv_obj_t* text = root->get<"text">().raw();
  check(!same(bg(text, LV_PART_CURSOR), red));
  lv_obj_add_state(text, LV_STATE_FOCUSED);
  check(same(bg(text, LV_PART_CURSOR), red));
}

// One name: lv::on("pressed", ...) is what .style_pressed does.
constexpr auto pressed_ui = lv::box[
    lv::button({.id = "on", .style = lv::on("pressed", {.radius = lv::raw(13 * px)})}),
    lv::button({.id = "slot", .style_pressed = {.radius = lv::raw(13 * px)}})];

void one_name_is_a_slot(lv::display& display) {
  auto root = lv::mount<pressed_ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  for (lv_obj_t* o : {root->get<"on">().raw(), root->get<"slot">().raw()}) {
    check(lv_obj_get_style_radius(o, LV_PART_MAIN) != 13);
    lv_obj_add_state(o, LV_STATE_PRESSED);
    check(lv_obj_get_style_radius(o, LV_PART_MAIN) == 13);
  }
}

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// The heap a mounted tree takes while it lives.
template <const auto& Tree>
std::size_t heap_of(lv::display& display) {
  const auto before = used_bytes();
  auto root = lv::mount<Tree>(display.screen());
  check(root.has_value());
  return used_bytes() - before;
}

constexpr auto knob = lv::style({.bg_color = lv::raw(red)});
constexpr auto plain_ui = lv::slider({.style = base});
constexpr auto slot_ui = lv::slider({.style = base, .style_knob = knob});
constexpr auto layer_ui = lv::slider({.style = base | lv::on("knob", knob)});
constexpr auto two_layers_ui = lv::slider({.style = base | lv::on("knob", knob) | knob_pressed});

// A layer costs what any added style costs: LVGL's entry in the widget's list of
// styles. The style itself is a constant.
void costs_one_list_entry(lv::display& display) {
  const auto plain = heap_of<plain_ui>(display);
  const auto slot = heap_of<slot_ui>(display);
  const auto layer = heap_of<layer_ui>(display);
  const auto layers = heap_of<two_layers_ui>(display);
  std::printf("heap of a slider: %zu bytes; with .style_knob %zu; with one layer %zu; with two %zu\n", plain, slot,
              layer, layers);
  check(layer == slot);
  check(layer > plain && layers > layer);
}

// ---- at run time ------------------------------------------------------------------

constexpr auto runtime_ui = lv::slider({.id = "slider"});

// A view sets a layered style again and again; the widget keeps one entry for it.
void set_at_run_time(lv::display& display) {
  auto root = lv::mount<runtime_ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  auto slider = root->get<"slider">();
  slider.set({.style = styled_knob});
  const auto once = used_bytes();
  slider.set({.style = styled_knob});
  check(used_bytes() == once);
  lv_obj_add_state(slider.raw(), LV_STATE_PRESSED);
  check(same(bg(slider.raw(), LV_PART_KNOB), red));
}

// The cases that break a contract: a layer in a style for another part, and two layered
// styles merged at run time.
int violates(const char* which) {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<runtime_ui>(display.screen());
  if (!root) return 1;
  if (std::strcmp(which, "part_slot") == 0) root->get<"slider">().set({.style_knob = styled_knob});
  if (std::strcmp(which, "merge") == 0) {
    style_type a = which[0] == 'm' ? styled_knob : two;
    style_type b = which[1] == 'e' ? two : styled_knob;
    root->get<"slider">().set({.style = a | b});
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1]));
    return 0;
  }
  lv::application app;
  {
    lv::display display{app, frame};
    slider_knob_pressed(display);
    indicator_when_checked(display);
    items_and_cursor(display);
    one_name_is_a_slot(display);
    costs_one_list_entry(display);
    set_at_run_time(display);
  }
  return failures;
}
