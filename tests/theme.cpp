// lv::theme: the styles every widget of a kind starts with, kept in read-only memory.

#include <lv/lv.hpp>
#include <lv/theme.hpp>

#include "check.hpp"

// The header says that it was read: a name that lv/detail/missing.hpp declares for it then takes the real definition.
static_assert(lv::detail::included<"lv/theme.hpp">);

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr lv_color_t ground{.blue = 12, .green = 11, .red = 10};
constexpr lv_color_t lime{.blue = 58, .green = 255, .red = 212};

constexpr auto mine = lv::theme({
    .arc = {.style = {.arc_width = lv::raw(8 * px)}, .style_indicator = {.arc_color = lv::raw(lime), .arc_width = lv::raw(6 * px)}},
    .bar = {.style = {.radius = lv::raw(1 * px)}},
    .button = {.style = {.bg_color = tok.color.primary,
                         .pad_all = lv::raw(20 * px),
                         .pad_left = lv::raw(3 * px),
                         .radius = lv::raw(0 * px),
                         .size = {lv::raw(90), lv::raw(40)}},
               .style_pressed = {.radius = lv::raw(5 * px)}},
    .obj = {.style = {.border_width = lv::raw(0 * px), .pad_gap = lv::raw(7 * px)}},
    .screen = {.style = {.bg_color = lv::raw(ground)}},
    .toggle = {.style = {.radius = lv::raw(2 * px)}},
});

constexpr auto nothing = lv::theme({});

static_assert(!mine.empty() && nothing.empty() && lv::theme_styles{}.empty());
static_assert(std::is_trivially_copyable_v<lv::theme_styles> && sizeof(lv::theme_styles) == 2 * sizeof(void*));

// Every style property without an LV_STYLE_* id is a shorthand that a theme can spell
// out. A new one in a later LVGL fails here, not in a user's theme.
// A template, because GCC 16.2 warns that `i` shadows itself in a plain function.
template <class D = void>
consteval bool every_shorthand_is_known() {
  template for (constexpr auto i : lv::detail::indices<lv::detail::style_members<D>.size()>) {
    if constexpr (lv::detail::style_ids<D>[i] == std::meta::info{})
      if (lv::detail::style_name<D>(i) != "enabled" &&  // not a value: see tests/fail/theme_not_a_value.cpp
          !lv::detail::is_shorthand<D>(i))
        return false;
  }
  return true;
}
static_assert(every_shorthand_is_known());

// A node of a tree still has the last word.
constexpr auto ui = lv::box[lv::button({.id = "plain"}), lv::button({.id = "own", .style = {.radius = lv::raw(9 * px)}})];

constinit lv::frame<64, 48> frame;
constinit lv::band<64, 48, 8> band;
constinit int sent = 0;

void changed(lv::area) {}
void send(lv::area, std::span<const std::byte>) { ++sent; }

struct frame_panel {
  void operator()(lv::area) {}
};
struct band_panel {
  void operator()(lv::area, std::span<const std::byte>) {}
};

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

std::int32_t radius(lv_obj_t* o, lv_part_t part = LV_PART_MAIN) { return lv_obj_get_style_radius(o, part); }

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

void styles_each_kind(lv::display& display) {
  lv_obj_t* screen = display.screen().raw();
  check(same(lv_obj_get_style_bg_color(screen, LV_PART_MAIN), ground));

  lv_obj_t* button = lv_button_create(screen);
  check(same(lv_obj_get_style_bg_color(button, LV_PART_MAIN), *tok.color.primary));
  check(radius(button) == 0);
  // pad_all stands for four properties; the one the theme names itself wins.
  check(lv_obj_get_style_pad_top(button, LV_PART_MAIN) == 20 && lv_obj_get_style_pad_bottom(button, LV_PART_MAIN) == 20);
  check(lv_obj_get_style_pad_right(button, LV_PART_MAIN) == 20 && lv_obj_get_style_pad_left(button, LV_PART_MAIN) == 3);
  // size has a value for each of its two.
  check(lv_obj_get_style_width(button, LV_PART_MAIN) == 90 && lv_obj_get_style_height(button, LV_PART_MAIN) == 40);
  // A state.
  lv_obj_add_state(button, LV_STATE_PRESSED);
  check(radius(button) == 5);

  // A part.
  lv_obj_t* arc = lv_arc_create(screen);
  check(lv_obj_get_style_arc_width(arc, LV_PART_MAIN) == 8 && lv_obj_get_style_arc_width(arc, LV_PART_INDICATOR) == 6);
  check(same(lv_obj_get_style_arc_color(arc, LV_PART_INDICATOR), lime));

  // `toggle` is LVGL's switch. A slider is not a bar: it keeps LVGL's look.
  check(radius(lv_switch_create(screen)) == 2);
  check(radius(lv_bar_create(screen)) == 1);
  check(radius(lv_slider_create(screen)) != 1);

  // A box in a screen is `obj`; a new screen is `screen`.
  lv_obj_t* box = lv_obj_create(screen);
  check(lv_obj_get_style_border_width(box, LV_PART_MAIN) == 0);
  check(lv_obj_get_style_pad_row(box, LV_PART_MAIN) == 7 && lv_obj_get_style_pad_column(box, LV_PART_MAIN) == 7);
  check(!same(lv_obj_get_style_bg_color(box, LV_PART_MAIN), ground));
  lv_obj_t* second = lv_obj_create(nullptr);
  check(same(lv_obj_get_style_bg_color(second, LV_PART_MAIN), ground));
  check(lv_obj_get_style_border_width(second, LV_PART_MAIN) == 0 || lv_obj_get_style_pad_row(second, LV_PART_MAIN) != 7);
  lv_obj_delete(second);
  lv_obj_clean(screen);
}

void a_node_overrides(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  check(radius(root->get<"plain">().raw()) == 0);
  check(radius(root->get<"own">().raw()) == 9);
}

// The styles are constants: a themed widget takes no more of LVGL's heap than a
// plain one, apart from the list of its styles.
void costs_no_heap_for_styles(lv::display& display) {
  lv_obj_t* screen = display.screen().raw();
  const auto before = used_bytes();
  lv_obj_t* arc = lv_arc_create(screen);
  const auto with_theme = used_bytes() - before;
  lv_obj_delete(arc);
  check(used_bytes() == before);
  check(with_theme < 512);
}

void with_theme() {
  lv::application app;
  { lv::display first{app, frame}; }  // LVGL makes its own theme once, and keeps it
  const auto start = used_bytes();
  {
    lv::display display{app, frame, mine};
    check(lv_display_get_theme(display.raw()) != lv_theme_default_get());
    styles_each_kind(display);
    a_node_overrides(display);
    costs_no_heap_for_styles(display);
  }
  check(used_bytes() == start);  // the display gave its theme back
}

void without_theme() {
  lv::application app;
  lv::display plain{app, frame};
  check(radius(lv_button_create(plain.screen().raw())) != 0);
  lv_theme_t* lvgls = lv_display_get_theme(plain.raw());

  // A theme that sets nothing changes nothing.
  lv::display empty{app, band, &send, nothing};
  check(lv_display_get_theme(empty.raw()) == lvgls);
}

// The theme is the last argument of every way to make a display.
void every_display_takes_one() {
  lv::application app;
  frame_panel on_frame;
  band_panel on_band;
  {
    lv::display display{app, frame, &changed, mine};
    check(radius(lv_button_create(display.screen().raw())) == 0);
  }
  {
    lv::display display{app, frame, on_frame, mine};
    check(radius(lv_button_create(display.screen().raw())) == 0);
  }
  {
    lv::display display{app, band, &send, mine};
    check(radius(lv_button_create(display.screen().raw())) == 0);
  }
  {
    lv::display display{app, band, on_band, mine};
    check(radius(lv_button_create(display.screen().raw())) == 0);
  }
}

}  // namespace

int main() {
  with_theme();
  without_theme();
  every_display_takes_one();
  return failures;
}
