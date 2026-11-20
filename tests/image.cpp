// lv::image_file: the compiler decodes a PNG file into pixels in read-only memory.
//
//   image <reference.png>

#include <lv/image.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <initializer_list>
#include <meta>
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
constexpr unsigned char row3_png[] = {
#embed "images/row3.png"
};  // 3 x 2 RGB, opaque: red, green, blue; then blue, green, red
constexpr unsigned char keyed_rgb_png[] = {
#embed "images/keyed_rgb.png"
};  // 3 x 2 RGB; the file names magenta as its see-through colour
constexpr unsigned char keyed_grey_png[] = {
#embed "images/keyed_grey.png"
};  // 3 x 1 grey: 7, 8, 9; the file names 8 as see-through
constexpr unsigned char chunked_png[] = {
#embed "images/chunked.png"
};  // row3.png with its compressed data in two IDAT chunks, and a chunk "Abcd" of 0xff bytes before them
constexpr unsigned char solid_png[] = {
#embed "images/solid.png"
};  // RGBA with every alpha at 255

constexpr std::size_t most = 128 * 128;

constexpr auto ring = lv::image_file<ring_png>;
constexpr auto flag = lv::image_file<flag_png>;
constexpr auto grey = lv::image_file<grey_png>;
constexpr auto grey_alpha = lv::image_file<grey_alpha_png>;
constexpr auto palette = lv::image_file<palette_png>;
constexpr auto noise = lv::image_file<noise_png>;
constexpr auto corner = lv::image_file<corner_png>;
constexpr auto solid = lv::image_file<solid_png>;
constexpr auto keyed_rgb = lv::image_file<keyed_rgb_png>;
constexpr auto keyed_grey = lv::image_file<keyed_grey_png>;
constexpr auto row3 = lv::image_file<row3_png>;
constexpr auto chunked = lv::image_file<chunked_png>;

// The same 3-pixel rows for an LVGL that wants rows of a multiple of four or eight bytes.
constexpr auto grey_by_4 = lv::detail::make_image(keyed_rgb_png, 16, most, 4);    // see-through: no padding
constexpr auto flag_by_4 = lv::detail::make_image(row3_png, 16, most, 4);         // 6 bytes become 8
constexpr auto flag_by_8 = lv::detail::make_image(row3_png, 32, most, 8);         // 12 bytes become 16

// The same files for a display with 32 bits for each pixel.
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

unsigned rgb565_at(const lv_image_dsc_t* image, unsigned stride, unsigned x, unsigned y) {
  const std::uint8_t* at = image->data + y * stride + 2 * x;
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

// A grey or RGB file can name one colour as see-through.
void honours_a_see_through_colour() {
  check(is(keyed_rgb, LV_COLOR_FORMAT_RGB565A8, 3, 2, 6, 18));
  const std::uint8_t* alpha = keyed_rgb->data + 12;
  check(alpha[0] == 0 && alpha[1] == 255 && alpha[2] == 255);  // magenta, red, green
  check(alpha[3] == 255 && alpha[4] == 0 && alpha[5] == 255);  // blue, magenta, nearly magenta
  check(is(keyed_grey, LV_COLOR_FORMAT_RGB565A8, 3, 1, 6, 9));
  check(keyed_grey->data[6] == 255 && keyed_grey->data[7] == 0 && keyed_grey->data[8] == 255);
}

// The pixels of a picture start where LVGL wants the data of a draw buffer to start, a multiple of
// LV_DRAW_BUF_ALIGN: it reads RGB565 pixels as 16-bit words. keyed_grey is RGB565A8 of 3 pixels,
// 9 bytes, so whatever the linker puts after it would start at an odd address without that.
// The compiler checks the alignment declared for the pixels, the same on every platform.
static_assert(std::meta::alignment_of(std::meta::substitute(
                  ^^lv::detail::stored_pixels,
                  {std::meta::reflect_constant(lv::detail::png_source{keyed_grey_png, sizeof keyed_grey_png}),
                   std::meta::reflect_constant(LV_COLOR_DEPTH),
                   std::meta::reflect_constant(std::uint32_t{LV_DRAW_BUF_STRIDE_ALIGN}),
                   std::meta::reflect_constant(std::size_t{9})})) %
                  LV_DRAW_BUF_ALIGN ==
              0);

// And where the linker put them.
void starts_pixels_where_lvgl_wants() {
  for (const lv_image_dsc_t* image : {ring, flag, grey, grey_alpha, palette, noise, corner, solid, keyed_rgb,
                                      keyed_grey, grey_by_4, flag_by_4, flag_by_8, ring32, flag32, palette32})
    check(reinterpret_cast<std::uintptr_t>(image->data) % LV_DRAW_BUF_ALIGN == 0);
}

// LVGL copies a picture to its heap when its rows are not as long as it wants them.
void pads_rows_to_what_lvgl_wants() {
  check(is(flag_by_4, LV_COLOR_FORMAT_RGB565, 3, 2, 8, 16));
  check(rgb565_at(flag_by_4, 8, 0, 0) == 0xf800 && rgb565_at(flag_by_4, 8, 2, 0) == 0x001f);
  check(rgb565_at(flag_by_4, 8, 0, 1) == 0x001f && rgb565_at(flag_by_4, 8, 2, 1) == 0xf800);
  check(flag_by_4->data[6] == 0 && flag_by_4->data[7] == 0);  // the padding
  check(is(flag_by_8, LV_COLOR_FORMAT_XRGB8888, 3, 2, 16, 32));
  check(flag_by_8->data[2] == 255 && flag_by_8->data[16] == 255);  // red first; blue first in the second row
  check(is(grey_by_4, LV_COLOR_FORMAT_RGB565A8, 3, 2, 6, 18));  // this form has no padding
}

// A file may cut its compressed data into several chunks, and carry chunks the reader does not know:
// the pixels are those of the same picture in one chunk.
void reads_a_file_cut_into_chunks() {
  check(is(chunked, LV_COLOR_FORMAT_RGB565, 3, 2, row3->header.stride, row3->data_size));
  check(sum(chunked) == sum(row3) && sum(row3) != 0);
  for (std::uint32_t i = 0; i < row3->data_size; ++i) check(chunked->data[i] == row3->data[i]);
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
  reads_a_file_cut_into_chunks();
  stores_for_32_bits();
  honours_a_see_through_colour();
  starts_pixels_where_lvgl_wants();
  pads_rows_to_what_lvgl_wants();
  decodes_long_codes_and_finds_alpha_anywhere();
  draws(argv[1]);
  draws_without_heap();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
