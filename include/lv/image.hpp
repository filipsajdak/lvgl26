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
// of a whole screen. So a picture is at most limits.compile_time_image of the settings
// pixels (128 x 128 by default); a larger one stops the build and names the setting.

#include <lv/config.hpp>
#include <lv/detail/settings.hpp>
#include <lv/detail/embedded.hpp>
#include <lv/detail/png_reader.hpp>
#include <lv/values.hpp>

#include <array>
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
    const unsigned char* row = p.bits() + y * (stride + 1) + 1;
    for (std::size_t x = 0; x < p.w; ++x)
      if (png_reader::pixel(p, row + x * p.step).a != 255) return true;
  }
  return false;
}

// What LVGL is told about stored pixels, and how many bytes they take.
struct image_layout {
  lv_color_format_t format;
  std::uint32_t stride;  // bytes in a row of the first plane
  std::size_t size;      // bytes of all planes
};

// The bytes of a row of `row` bytes, when rows start at a multiple of `align`.
consteval std::uint32_t aligned(std::uint32_t row, std::uint32_t align) { return (row + align - 1) / align * align; }

// How a picture is stored: sixteen bits for each pixel, or thirty-two, with an alpha plane
// (RGB565A8) or an alpha byte (ARGB8888) when some pixel is see-through.
consteval image_layout layout_for(std::uint32_t w, std::uint32_t h, bool alpha, int depth, std::uint32_t align) {
  if (depth != 16) {
    const std::uint32_t stride = aligned(w * 4, align);
    return {alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_XRGB8888, stride, static_cast<std::size_t>(stride) * h};
  }
  const std::uint32_t stride = alpha ? w * 2 : aligned(w * 2, align);
  return {alpha ? LV_COLOR_FORMAT_RGB565A8 : LV_COLOR_FORMAT_RGB565, stride,
          static_cast<std::size_t>(stride) * h + (alpha ? static_cast<std::size_t>(w) * h : 0)};
}

// Sixteen bits for each pixel, the low byte first. With `alpha`, a byte of alpha for
// each pixel follows the colours, and rows have no padding: LVGL asks none of that form.
consteval void write_rgb565(const png_reader::picture& p, bool alpha, std::uint32_t stride, std::uint8_t* out) {
  const std::size_t from = p.w * p.step;
  std::uint8_t* mask = out + static_cast<std::size_t>(stride) * p.h;
  for (std::size_t y = 0; y < p.h; ++y) {
    const unsigned char* at = p.bits() + y * (from + 1) + 1;
    std::uint8_t* colour = out + y * stride;
    for (std::size_t x = 0; x < p.w; ++x, at += p.step) {
      const auto c = png_reader::pixel(p, at);
      const unsigned v = (c.r >> 3) << 11 | (c.g >> 2) << 5 | c.b >> 3;
      *colour++ = static_cast<std::uint8_t>(v), *colour++ = static_cast<std::uint8_t>(v >> 8);
      if (alpha) *mask++ = c.a;
    }
  }
}

// Four bytes for each pixel: blue, green, red, then alpha.
consteval void write_argb8888(const png_reader::picture& p, std::uint32_t stride, std::uint8_t* out) {
  const std::size_t from = p.w * p.step;
  for (std::size_t y = 0; y < p.h; ++y) {
    const unsigned char* at = p.bits() + y * (from + 1) + 1;
    std::uint8_t* to = out + y * stride;
    for (std::size_t x = 0; x < p.w; ++x, at += p.step) {
      const auto c = png_reader::pixel(p, at);
      *to++ = c.b, *to++ = c.g, *to++ = c.r, *to++ = c.a;
    }
  }
}

// A PNG file, as a template argument: the address of its bytes, which is a short name. A symbol made
// from the pixels would be as long as they are, and Apple's linker stops at a million characters.
struct png_source {
  const unsigned char* data;
  std::size_t size;
};

consteval png_reader::bytes bytes_of(png_source file) { return {file.data, file.size}; }

// The rows of the picture with their filters undone, filled through a pointer.
// A call with constant arguments inside a template runs twice in GCC 16.2, so each step
// below takes its arguments as parameters of a function, and the variable templates only call.
template <std::size_t Size>
consteval std::array<unsigned char, Size> rows_of(png_source file) {
  std::array<unsigned char, Size> out{};
  const auto rows = png_reader::decode(bytes_of(file)).rows;
  unsigned char* to = out.data();
  const unsigned char* from = rows.data();
  for (std::size_t i = 0; i < Size; ++i) to[i] = from[i];
  return out;
}

// In a constant evaluation of its own and as a named variable: the layout and the pixels read it,
// so the file is decoded once.
template <png_source File>
inline constexpr std::array<unsigned char, png_reader::rows_size(bytes_of(File))> decoded_rows =
    rows_of<png_reader::rows_size(bytes_of(File))>(File);

consteval image_layout layout_of(png_source file, const unsigned char* rows, int depth, std::uint32_t align) {
  const auto p = png_reader::picture_from(bytes_of(file), rows);
  return layout_for(p.w, p.h, has_alpha(p), depth, align);
}

template <png_source File, int Depth, std::uint32_t Align>
inline constexpr image_layout layout = layout_of(File, decoded_rows<File>.data(), Depth, Align);

// The stored pixels, written through a pointer into an array of their own.
template <std::size_t Size>
consteval std::array<std::uint8_t, Size> pixels_of(png_source file, const unsigned char* rows, image_layout form,
                                                   int depth) {
  const auto p = png_reader::picture_from(bytes_of(file), rows);
  std::array<std::uint8_t, Size> out{};
  if (depth == 16)
    write_rgb565(p, form.format == LV_COLOR_FORMAT_RGB565A8, form.stride, out.data());
  else
    write_argb8888(p, form.stride, out.data());
  return out;
}

template <png_source File, int Depth, std::uint32_t Align, std::size_t Size>
inline constexpr std::array<std::uint8_t, Size> stored_pixels =
    pixels_of<Size>(File, decoded_rows<File>.data(), layout<File, Depth, Align>, Depth);

template <png_source File, int Depth, std::uint32_t Align, std::size_t Size>
inline constexpr const std::uint8_t* stored_data = stored_pixels<File, Depth, Align, Size>.data();

consteval const std::uint8_t* stored_pixels_of(png_source file, int depth, std::uint32_t align, std::size_t size) {
  using std::meta::reflect_constant;
  return std::meta::extract<const std::uint8_t*>(std::meta::substitute(
      ^^stored_data, {reflect_constant(file), reflect_constant(depth), reflect_constant(align), reflect_constant(size)}));
}

consteval image_layout stored_layout(png_source file, int depth, std::uint32_t align) {
  using std::meta::reflect_constant;
  return std::meta::extract<image_layout>(std::meta::substitute(
      ^^layout, {reflect_constant(file), reflect_constant(depth), reflect_constant(align)}));
}

// LVGL keeps the width, the height and the bytes of a row in 16 bits each.
consteval void check_size(std::uint32_t w, std::uint32_t h, std::uint32_t stride, std::size_t most) {
  if (w > 0xffff || h > 0xffff || stride > 0xffff)
    fail("lv: this picture is " + decimal(w) + " x " + decimal(h) + " pixels, with rows of " + decimal(stride) +
         " bytes. LVGL holds a picture of at most 65535 pixels each way, and rows of at most 65535 bytes.");
  if (static_cast<std::size_t>(w) * h > most)
    fail("lv: this picture is " + decimal(w) + " x " + decimal(h) + " pixels, and the compiler decodes at most " + decimal(most) +
         " (a large picture makes the build slow). To allow it, raise limits.compile_time_image in the settings.");
}

// The picture of the PNG file `file` as a constant LVGL image. `depth` is the
// display's colour depth, `most` the largest picture the project allows, and `align`
// what LVGL wants the bytes of a row to be a multiple of.
consteval const lv_image_dsc_t* make_image(png_reader::bytes file, int depth, std::size_t most,
                                           std::uint32_t align = LV_DRAW_BUF_STRIDE_ALIGN) {
  const std::uint32_t w = png_reader::width_of(file), h = png_reader::height_of(file);
  check_size(w, h, aligned(w * (depth == 16 ? 2 : 4), align), most);
  const png_source source{file.data(), file.size()};
  const image_layout form = stored_layout(source, depth, align);
  lv_image_dsc_t dsc{};
  dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  dsc.header.cf = static_cast<std::uint32_t>(form.format) & 0xff;
  dsc.header.w = w & 0xffff, dsc.header.h = h & 0xffff;
  dsc.header.stride = form.stride & 0xffff;
  dsc.data_size = static_cast<std::uint32_t>(form.size);
  dsc.data = stored_pixels_of(source, depth, align, form.size);
  return std::define_static_array(std::vector{dsc}).data();
}

// O only delays reading the project's setting until an image is made.
template <const auto& File, class O = override>
inline constexpr const lv_image_dsc_t* image_of = make_image(File, LV_COLOR_DEPTH, detail::project<O>.limits.compile_time_image.value);

}  // namespace detail

// The picture of the embedded PNG file `File`, decoded by the compiler: what an
// image widget takes as its `.src`.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* image_file = detail::image_of<File>;

}  // namespace lv
