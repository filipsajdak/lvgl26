// Two files of one program name the same looks: the registration is in a header they share,
// and the constants of a look are made in each file without clashing at link time.
#include "look_link.hpp"

#include "check.hpp"

namespace {

constexpr auto tile = lv::button({.style = lv::look<"tile"> | lv::look<"tile-print">});

unsigned rgb(lv_color_t c) { return lv_color_to_u32(c) & 0xffffffu; }

}  // namespace

int main() {
  lv::application app;
  constinit static lv::frame<320, 240> frame;
  lv::display display{app, frame};
  auto here = lv::mount<tile>(display.screen());
  check(here.has_value());
  const lv::obj there = mount_card(display.screen());
  check(static_cast<bool>(there));
  if (!here || !there) return 1;
  check(rgb(lv_obj_get_style_bg_color(here->root().raw(), LV_PART_MAIN)) == 0x2b7fff);
  check(rgb(lv_obj_get_style_bg_color(there.raw(), LV_PART_MAIN)) == 0x2b7fff);
  check(lv_obj_get_style_radius(here->root().raw(), LV_PART_MAIN) == 24);
  check(lv_obj_get_style_radius(there.raw(), LV_PART_MAIN) == 12);  // card's own 12px, before tile-print
  return failures;
}
