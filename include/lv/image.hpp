#pragma once

// Pictures in the program: a PNG file, embedded at compile time.
//
//   constexpr unsigned char print_png[] = {
//   #embed "images/print.png"
//   };
//   constexpr lv_image_dsc_t print_icon = lv::png(print_png);
//
//   lv::image({.src = &print_icon})
//
// lv::png checks that the bytes are a PNG file and reads its size from them, at compile
// time; a file that is not a PNG is a compile error. The bytes stay as they are, in
// read-only memory, and LVGL decodes them when the picture is first drawn, so LVGL must
// be built with a PNG decoder (LV_USE_LODEPNG or LV_USE_LIBPNG).
// The array must outlive the picture: make it a constant at namespace scope, as above.

#include <lv/detail/api.hpp>

#include <cstddef>
#include <cstdint>

namespace lv {

namespace detail {

// Whether these bytes start as every PNG file does.
template <std::size_t N>
constexpr bool is_png(const unsigned char (&bytes)[N]) {
  constexpr unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  if (N < 24) return false;  // the signature and the start of the header with the size
  for (std::size_t i = 0; i < sizeof signature; ++i)
    if (bytes[i] != signature[i]) return false;
  return true;
}

// The four bytes at `at`, the first one counting most: how PNG writes a number.
template <std::size_t N>
constexpr std::uint32_t png_number(const unsigned char (&bytes)[N], std::size_t at) {
  return (std::uint32_t{bytes[at]} << 24) | (std::uint32_t{bytes[at + 1]} << 16) |
         (std::uint32_t{bytes[at + 2]} << 8) | std::uint32_t{bytes[at + 3]};
}

}  // namespace detail

// The picture in an embedded PNG file. See the top of this file.
template <std::size_t N>
consteval lv_image_dsc_t png(const unsigned char (&bytes)[N]) {
  if (!detail::is_png(bytes))
    throw detail::unknown_name("lv::png: these bytes are not a PNG file. Embed the file itself, with #embed.");
  lv_image_dsc_t picture{};
  picture.header.magic = LV_IMAGE_HEADER_MAGIC;
  picture.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
  const std::uint32_t width = detail::png_number(bytes, 16);
  const std::uint32_t height = detail::png_number(bytes, 20);
  if (width > 0xffff || height > 0xffff)
    throw detail::unknown_name("lv::png: the picture is larger than 65535 pixels on a side, which LVGL cannot hold.");
  picture.header.w = static_cast<std::uint16_t>(width);
  picture.header.h = static_cast<std::uint16_t>(height);
  picture.data_size = N;
  picture.data = bytes;
  return picture;
}

}  // namespace lv
