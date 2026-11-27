// detail::restyle: the styles of one theme take the place of another's in the widgets that exist, in
// the order they stand in the list of each widget, and without a byte of heap.

#include <lv/lv.hpp>
#include <lv/theme.hpp>

#include "check.hpp"

#include <cstddef>

namespace {

using namespace lv::units;

constexpr lv_color_t day_ground{.blue = 240, .green = 240, .red = 240};
constexpr lv_color_t night_ground{.blue = 12, .green = 11, .red = 10};
constexpr lv_color_t day_key{.blue = 255, .green = 100, .red = 20};
constexpr lv_color_t night_key{.blue = 20, .green = 200, .red = 90};
constexpr lv_color_t own_color{.blue = 1, .green = 2, .red = 3};

// Two themes of one shape: the same widgets and selectors, other values.
constexpr auto day = lv::theme({
    .arc = {.style_indicator = {.arc_color = lv::raw(day_key)}},
    .button = {.style = {.bg_color = lv::raw(day_key)}, .style_pressed = {.radius = lv::raw(5 * px)}},
    .screen = {.style = {.bg_color = lv::raw(day_ground)}},
});
constexpr auto night = lv::theme({
    .arc = {.style_indicator = {.arc_color = lv::raw(night_key)}},
    .button = {.style = {.bg_color = lv::raw(night_key)}, .style_pressed = {.radius = lv::raw(9 * px)}},
    .screen = {.style = {.bg_color = lv::raw(night_ground)}},
});

constexpr auto ui = lv::column[lv::button({.id = "plain"}), lv::button({.id = "own", .style = {.bg_color = lv::raw(own_color)}}),
                               lv::group[lv::button({.id = "inner"})], lv::arc({.id = "ring"})];

constinit lv::frame<64, 48> frame;

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }
lv_color_t bg(lv_obj_t* o) { return lv_obj_get_style_bg_color(o, LV_PART_MAIN); }

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

void the_widgets_change_in_place(lv::display& display) {
  auto root = lv::mount<ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  lv_obj_t* screen = display.screen().raw();
  lv_obj_t* plain = root->get<"plain">().raw();
  lv_obj_t* own = root->get<"own">().raw();
  lv_obj_t* inner = root->get<"inner">().raw();
  lv_obj_t* ring = root->get<"ring">().raw();
  check(same(bg(screen), day_ground) && same(bg(plain), day_key));
  check(same(lv_obj_get_style_arc_color(ring, LV_PART_INDICATOR), day_key));

  const auto before = used_bytes();
  lv::detail::restyle(screen, lv::detail::theme_access::first(day), lv::detail::theme_access::first(night));
  check(used_bytes() == before);  // nothing was allocated or freed

  check(same(bg(screen), night_ground) && same(bg(plain), night_key));
  check(same(lv_obj_get_style_arc_color(ring, LV_PART_INDICATOR), night_key));
  // A state of a part is swapped too.
  lv_obj_add_state(plain, LV_STATE_PRESSED);
  check(lv_obj_get_style_radius(plain, LV_PART_MAIN) == 9);
  lv_obj_remove_state(plain, LV_STATE_PRESSED);
  // A node's own style was added after the theme and still wins; a button in a group is a button.
  check(same(bg(own), own_color));
  check(same(bg(inner), night_key));

  // And back.
  lv::detail::restyle(screen, lv::detail::theme_access::first(night), lv::detail::theme_access::first(day));
  check(same(bg(screen), day_ground) && same(bg(plain), day_key) && same(bg(own), own_color));
  check(used_bytes() == before);
  // The same theme twice changes nothing.
  lv::detail::restyle(screen, lv::detail::theme_access::first(day), lv::detail::theme_access::first(day));
  check(same(bg(plain), day_key));
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame, day};
  the_widgets_change_in_place(display);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
