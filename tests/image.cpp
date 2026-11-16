// lv::image_file: the compiler decodes a PNG file into pixels in read-only memory.
//
//   image <reference.png>

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstdio>
#include <span>

namespace {

using namespace lv::units;

// Each file is one kind of PNG; tests/data/images says how they were made.
constexpr unsigned char ring_png[] = {
#embed "images/ring.png"
};  // RGBA, its rows use all five filters
constexpr unsigned char flag_png[] = {
#embed "images/flag.png"
};  // RGB: red, white, blue
constexpr unsigned char grey_png[] = {
#embed "images/grey.png"
};
constexpr unsigned char grey_alpha_png[] = {
#embed "images/grey_alpha.png"
};
constexpr unsigned char palette_png[] = {
#embed "images/palette.png"
};  // three colours; the first two have an alpha of 255 and 128

constexpr unsigned char noise_png[] = {
#embed "images/noise.png"
};  // uneven noise: its compressed data has codes longer than ten bits
constexpr unsigned char corner_png[] = {
#embed "images/corner.png"
};  // opaque but for its last pixel
constexpr unsigned char solid_png[] = {
#embed "images/solid.png"
};  // RGBA with every alpha at 255

constexpr auto ring = lv::image_file<ring_png>;
constexpr auto flag = lv::image_file<flag_png>;
constexpr auto grey = lv::image_file<grey_png>;
constexpr auto grey_alpha = lv::image_file<grey_alpha_png>;
constexpr auto palette = lv::image_file<palette_png>;
constexpr auto noise = lv::image_file<noise_png>;
constexpr auto corner = lv::image_file<corner_png>;
constexpr auto solid = lv::image_file<solid_png>;

// The same files for a display with 32 bits for each pixel.
constexpr std::size_t most = 128 * 128;
constexpr auto ring32 = lv::detail::make_image(ring_png, 32, most);
constexpr auto flag32 = lv::detail::make_image(flag_png, 32, most);
constexpr auto palette32 = lv::detail::make_image(palette_png, 32, most);

static_assert(lv::detail::png_reader::width_of(flag_png) == 24 && lv::detail::png_reader::height_of(flag_png) == 16);

constexpr auto ui = lv::box({.size = {320 * px, 240 * px}})[
    lv::image({.id = "ring", .pos = {20 * px, 20 * px}, .src = ring}),
    lv::image({.id = "flag", .pos = {80 * px, 20 * px}, .src = flag}),
    lv::image({.id = "palette", .pos = {120 * px, 20 * px}, .src = palette})];

unsigned long sum(const lv_image_dsc_t* image) {
  unsigned long s = 0;
  for (const auto byte : std::span{image->data, image->data_size}) s += byte;
  return s;
}

bool is(const lv_image_dsc_t* image, lv_color_format_t format, unsigned w, unsigned h, unsigned stride, unsigned bytes) {
  return image->header.magic == LV_IMAGE_HEADER_MAGIC && image->header.cf == format && image->header.w == w &&
         image->header.h == h && image->header.stride == stride && image->data_size == bytes;
}

unsigned rgb565(const lv_image_dsc_t* image, unsigned x, unsigned y) {
  const std::uint8_t* at = image->data + 2 * (y * image->header.w + x);
  return static_cast<unsigned>(at[0] | at[1] << 8);
}

// The sums were worked out from the files by a reference decoder.
void stores_for_16_bits() {
  check(is(flag, LV_COLOR_FORMAT_RGB565, 24, 16, 48, 24 * 16 * 2) && sum(flag) == 100992);
  check(rgb565(flag, 0, 0) == 0xf800 && rgb565(flag, 8, 15) == 0xffff && rgb565(flag, 23, 7) == 0x001f);
  check(is(grey, LV_COLOR_FORMAT_RGB565, 8, 8, 16, 128) && sum(grey) == 9312);
  // A see-through picture has a byte of alpha for each pixel after the colours.
  check(is(ring, LV_COLOR_FORMAT_RGB565A8, 32, 32, 64, 32 * 32 * 3) && sum(ring) == 568888);
  check(ring->data[32 * 32 * 2] == 0 && ring->data[32 * 32 * 2 + 16 * 32 + 5] == 255);  // a corner, the ring
  check(is(grey_alpha, LV_COLOR_FORMAT_RGB565A8, 8, 8, 16, 192) && sum(grey_alpha) == 26048);
  check(is(palette, LV_COLOR_FORMAT_RGB565A8, 8, 8, 16, 192) && sum(palette) == 24467);
  check(rgb565(palette, 0, 0) == 0xf800 && rgb565(palette, 1, 0) == 0x07e0 && rgb565(palette, 2, 0) == 0x001f);
  check(palette->data[128] == 255 && palette->data[129] == 128 && palette->data[130] == 255);
}

void decodes_long_codes_and_finds_alpha_anywhere() {
  check(is(noise, LV_COLOR_FORMAT_RGB565, 64, 64, 128, 64 * 64 * 2) && sum(noise) == 798627);
  // A file with an alpha channel that is opaque everywhere needs no alpha plane.
  check(is(solid, LV_COLOR_FORMAT_RGB565, 8, 8, 16, 128));
  check(is(corner, LV_COLOR_FORMAT_RGB565A8, 8, 8, 16, 192) && corner->data[128 + 63] == 0 && corner->data[128 + 62] == 255);
}

void stores_for_32_bits() {
  check(is(flag32, LV_COLOR_FORMAT_XRGB8888, 24, 16, 96, 24 * 16 * 4) && sum(flag32) == 261120);
  check(flag32->data[0] == 0 && flag32->data[2] == 255);  // blue first, red third
  check(is(ring32, LV_COLOR_FORMAT_ARGB8888, 32, 32, 128, 32 * 32 * 4) && sum(ring32) == 649784);
  check(is(palette32, LV_COLOR_FORMAT_ARGB8888, 8, 8, 32, 256) && sum(palette32) == 29846);
}

void draws(const char* reference) {
  auto root = lv::mount<ui>(lv::screen_active());
  check(root.has_value());
  if (!root) return;
  check(lv_image_get_src(root->get<"ring">().raw()) == ring);
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_width(root->get<"flag">().raw()) == 24 && lv_obj_get_height(root->get<"flag">().raw()) == 16);
  check(lv_test_screenshot_compare(reference) == LV_TEST_SCREENSHOT_RESULT_PASSED);
}

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// The pixels are constants: drawing a picture leaves nothing on LVGL's heap.
void draws_without_heap() {
  const auto before = used_bytes();
  {
    auto root = lv::mount<ui>(lv::screen_active());
    lv_refr_now(nullptr);
  }
  lv_refr_now(nullptr);
  check(used_bytes() == before);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: image <reference.png>\n");
    return 2;
  }
  lv_init();
  lv_test_display_create(320, 240);
  stores_for_16_bits();
  stores_for_32_bits();
  decodes_long_codes_and_finds_alpha_anywhere();
  draws(argv[1]);
  draws_without_heap();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
