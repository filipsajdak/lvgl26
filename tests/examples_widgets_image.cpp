// The image examples of LVGL's examples/widgets, rewritten with lvgl26, must render
// the same pixels as the C originals.

#include "../examples/lvgl/widgets/widgets.hpp"

#include "same_pixels.hpp"

extern "C" {
void lv_example_image_inner_align(void);
void lv_example_image_src(void);
void lv_example_image_transformations(void);
}

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("image_src", &lv_example_image_src, &widgets::image_src, &nothing);
  same_pixels("image_inner_align", &lv_example_image_inner_align, &widgets::image_inner_align,
              &nothing);
  same_pixels("image_transformations", &lv_example_image_transformations,
              &widgets::image_transformations, &nothing);

  // Placing the logo in a tile, or turning it, must give another picture than the logo alone.
  const auto plain = render(&widgets::image_src, &nothing);
  const auto tiled = render(&widgets::image_inner_align, &nothing);
  const auto turned = render(&widgets::image_transformations, &nothing);
  check(plain.first_difference(tiled) != -1);
  check(plain.first_difference(turned) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
