#pragma once

// Images: the compiler decodes a PNG file and stores its pixels in read-only memory,
// in the form the display draws fastest. The device needs no decoder and no heap.
//
//   constexpr unsigned char logo_png[] = {
//   #embed "logo.png"
//   };
//   constexpr auto logo = lv::image_file<logo_png>;
//
//   lv::image({.src = logo})
//
// The stored form follows the picture and the display (LVGL's colour depth):
//
//   every pixel opaque     the display's own pixels: RGB565, or XRGB8888 at 32 bits
//   some pixels see-through   RGB565 with an alpha plane, or ARGB8888 at 32 bits
//
// Decoding costs compile time: under a second for an icon, seconds for a picture
// of a whole screen. So a picture is at most lv::config::compile_time_image_pixels
// pixels (128 x 128 by default); a larger one stops the build and names the setting.

#include <lv/config.hpp>
#include <lv/detail/embedded.hpp>
#include <lv/detail/png_reader.hpp>

#include <cstddef>
#include <cstdint>
#include <meta>
#include <span>
#include <vector>

namespace lv {

namespace detail {

consteval bool has_alpha(const png_reader::picture& p) {
  const std::size_t stride = p.w * p.step;
  for (std::size_t y = 0; y < p.h; ++y) {
    const unsigned char* row = p.rows.data() + y * (stride + 1) + 1;
    for (std::size_t x = 0; x < p.w; ++x)
      if (png_reader::pixel(p, row + x * p.step).a != 255) return true;
  }
  return false;
}

// What LVGL is told about stored pixels.
struct stored_pixels {
  lv_color_format_t format;
  std::uint32_t stride;  // bytes in a row of the first plane
  std::vector<std::uint8_t> data;
};

// Sixteen bits for each pixel, the low byte first; then, when `alpha`, a byte of alpha for each pixel.
consteval stored_pixels as_rgb565(const png_reader::picture& p, bool alpha) {
  const std::size_t n = static_cast<std::size_t>(p.w) * p.h, stride = p.w * p.step;
  stored_pixels out{alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565, p.w * 2, std::vector<std::uint8_t>(n * (alpha ? 3 : 2))};
  std::uint8_t* colour = out.data.data();
  std::uint8_t* mask = colour + n * 2;
  for (std::size_t y = 0; y < p.h; ++y) {
    const unsigned char* at = p.rows.data() + y * (stride + 1) + 1;
    for (std::size_t x = 0; x < p.w; ++x, at += p.step) {
      const auto c = png_reader::pixel(p, at);
      const unsigned v = (c.r >> 3) << 11 | (c.g >> 2) << 5 | c.b >> 3;
      *colour++ = static_cast<std::uint8_t>(v), *colour++ = static_cast<std::uint8_t>(v >> 8);
      if (alpha) *mask++ = c.a;
    }
  }
  return out;
}

// Four bytes for each pixel: blue, green, red, then alpha.
consteval stored_pixels as_argb8888(const png_reader::picture& p, bool alpha) {
  const std::size_t stride = p.w * p.step;
  stored_pixels out{alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_XRGB8888, p.w * 4,
                    std::vector<std::uint8_t>(static_cast<std::size_t>(p.w) * p.h * 4)};
  std::uint8_t* to = out.data.data();
  for (std::size_t y = 0; y < p.h; ++y) {
    const unsigned char* at = p.rows.data() + y * (stride + 1) + 1;
    for (std::size_t x = 0; x < p.w; ++x, at += p.step) {
      const auto c = png_reader::pixel(p, at);
      *to++ = c.b, *to++ = c.g, *to++ = c.r, *to++ = c.a;
    }
  }
  return out;
}

consteval void check_size(std::uint32_t w, std::uint32_t h, std::size_t most) {
  if (static_cast<std::size_t>(w) * h > most)
    fail("lv: this picture is " + decimal(w) + " x " + decimal(h) + " pixels, and the compiler decodes at most " + decimal(most) +
         " (a large picture makes the build slow). To allow it, raise lv::config::compile_time_image_pixels.");
}

// The picture of the PNG file `file` as a constant LVGL image. `depth` is the
// display's colour depth, `most` the largest picture the project allows.
consteval const lv_image_dsc_t* make_image(png_reader::bytes file, int depth, std::size_t most) {
  check_size(png_reader::width_of(file), png_reader::height_of(file), most);
  const auto p = png_reader::decode(file);
  const bool alpha = has_alpha(p);
  const auto pixels = depth == 16 ? as_rgb565(p, alpha) : as_argb8888(p, alpha);
  lv_image_dsc_t dsc{};
  dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  dsc.header.cf = static_cast<std::uint32_t>(pixels.format) & 0xff;
  dsc.header.w = p.w & 0xffff, dsc.header.h = p.h & 0xffff;
  dsc.header.stride = pixels.stride & 0xffff;
  dsc.data_size = static_cast<std::uint32_t>(pixels.data.size());
  dsc.data = std::define_static_array(pixels.data).data();
  return std::define_static_array(std::vector{dsc}).data();
}

// O only delays reading the project's setting until an image is made.
template <const auto& File, class O = override>
inline constexpr const lv_image_dsc_t* image_of = make_image(File, LV_COLOR_DEPTH, cfg<O>.compile_time_image_pixels);

}  // namespace detail

// The picture of the embedded PNG file `File`, decoded by the compiler: what an
// image widget takes as its `.src`.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* image_file = detail::image_of<File>;

}  // namespace lv
