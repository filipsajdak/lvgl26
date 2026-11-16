// lv::encoded_image: a PNG or JPEG file kept as it is, and decoded on the device.
//
//   encoded_image <reference.png>

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdio>

namespace {

using namespace lv::units;

constexpr unsigned char ring_png[] = {
#embed "images/ring.png"
};  // 32 x 32, see-through
constexpr unsigned char gradient_jpg[] = {
#embed "images/gradient.jpg"
};  // 48 x 32, baseline: red grows to the right, green downwards

constexpr unsigned char wide_jpg[] = {
#embed "images/wide.jpg"
};  // 300 x 8: a size that needs both bytes of its number
constexpr unsigned char wide_png[] = {
#embed "images/wide_grey.png"
};  // 300 x 2
constexpr unsigned char tallest_jpg[] = {
#embed "images/tallest.jpg"
};  // only a header, which says 1 x 65535: the most LVGL holds

constexpr auto ring = lv::encoded_image<ring_png>;
constexpr auto gradient = lv::encoded_image<gradient_jpg>;

constexpr auto ui = lv::box({.size = {320 * px, 240 * px}})[
    lv::image({.id = "ring", .pos = {20 * px, 20 * px}, .src = ring}),
    lv::image({.id = "gradient", .pos = {80 * px, 20 * px}, .src = gradient})];

// The image is the file: its size is read from it, and its data is the program's array.
void names_the_file() {
  check(ring->header.magic == LV_IMAGE_HEADER_MAGIC && ring->header.cf == LV_COLOR_FORMAT_RAW_ALPHA);
  check(ring->header.w == 32 && ring->header.h == 32);
  check(ring->data == ring_png && ring->data_size == sizeof(ring_png));
  check(gradient->header.cf == LV_COLOR_FORMAT_RAW && gradient->header.w == 48 && gradient->header.h == 32);
  check(gradient->data == gradient_jpg && gradient->data_size == sizeof(gradient_jpg));
}

void reads_sizes_of_two_bytes() {
  static constexpr auto wide = lv::encoded_image<wide_jpg>;
  static constexpr auto wide_grey = lv::encoded_image<wide_png>;
  static constexpr auto tallest = lv::encoded_image<tallest_jpg>;
  check(wide->header.w == 300 && wide->header.h == 8);
  check(wide_grey->header.w == 300 && wide_grey->header.h == 2);
  check(tallest->header.w == 1 && tallest->header.h == 65535);
}

void is_decoded_and_drawn(const char* reference) {
  auto root = lv::mount<ui>(lv::screen_active());
  check(root.has_value());
  if (!root) return;
  lv_obj_update_layout(lv_screen_active());
  // LVGL's decoders accepted both files: the widgets have the pictures' sizes.
  check(lv_obj_get_width(root->get<"ring">().raw()) == 32 && lv_obj_get_height(root->get<"ring">().raw()) == 32);
  check(lv_obj_get_width(root->get<"gradient">().raw()) == 48 && lv_obj_get_height(root->get<"gradient">().raw()) == 32);
  check(lv_test_screenshot_compare(reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: encoded_image <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  names_the_file();
  reads_sizes_of_two_bytes();
  is_decoded_and_drawn(argv[1]);
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
