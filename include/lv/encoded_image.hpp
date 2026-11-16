#pragma once

// A picture kept as its file: the bytes of a PNG or JPEG stay in flash as they are,
// and LVGL's decoder draws them on the device.
//
//   constexpr unsigned char photo_jpg[] = {
//   #embed "photo.jpg"
//   };
//   constexpr auto photo = lv::encoded_image<photo_jpg>;
//
//   lv::image({.src = photo})
//
// Use it for a photo, or for a picture too large for the compiler to decode. A file is
// much smaller than its pixels, and the device pays for that: the decoder's code, and
// heap while the picture is drawn. LVGL decodes a PNG into four bytes for each pixel
// on its heap; a JPEG in small blocks. Without LVGL's image cache it decodes again
// each time the picture is drawn.
//
// The decoders are off by default. Turn them on in CMake, before lvgl26 is added:
// `set(LVGL26_PNG ON)`, `set(LVGL26_JPEG ON)`. A picture that the compiler decodes
// needs neither: see lv/image.hpp.

#include <lv/detail/api.hpp>
#include <lv/detail/embedded.hpp>
#include <lv/values.hpp>

#include <cstddef>
#include <cstdint>
#include <meta>
#include <span>
#include <vector>

namespace lv {

namespace detail {

using file_bytes = std::span<const unsigned char>;

// LVGL's decoders, or null reflections when LVGL is built without them. D only delays
// the search until a program keeps a picture as a file.
template <class D = void>
inline constexpr std::meta::info png_decoder = find("lv_lodepng_init");
template <class D = void>
inline constexpr std::meta::info jpeg_decoder = find("lv_tjpgd_init");

consteval bool starts_with(file_bytes d, std::initializer_list<unsigned char> signature) {
  std::size_t i = 0;
  for (const unsigned char c : signature)
    if (i >= d.size() || d[i++] != c) return false;
  return true;
}

consteval bool is_png(file_bytes d) { return starts_with(d, {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'}) && d.size() >= 24; }
consteval bool is_jpeg(file_bytes d) { return starts_with(d, {0xff, 0xd8, 0xff}); }

consteval std::uint32_t big16(file_bytes d, std::size_t at) { return static_cast<std::uint32_t>(d[at] << 8 | d[at + 1]); }

struct picture_size {
  std::uint32_t w, h;
};

// The markers that begin a frame header: 0xc0 to 0xcf, but for three that mean other things.
consteval bool is_frame_marker(unsigned char marker) {
  return marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc;
}

// Where the frame header of a JPEG file starts: the marker that gives its size.
// Other parts of the file are skipped by their length.
consteval std::size_t jpeg_frame(file_bytes d) {
  std::size_t at = 2;
  while (at + 9 < d.size() && d[at] == 0xff) {
    const unsigned char marker = d[at + 1];
    if (is_frame_marker(marker)) return at;
    if (marker == 0xda) break;  // the picture's data begins: no size came before it
    at += 2 + big16(d, at + 2);
  }
  fail("lv: this JPEG file is damaged: it does not say how large its picture is.");
}

// The size of a JPEG picture. LVGL's decoder reads the plain form only.
consteval picture_size jpeg_size(file_bytes d) {
  const std::size_t frame = jpeg_frame(d);
  if (d[frame + 1] != 0xc0)
    fail("lv: this JPEG file is progressive, or of another form than baseline, and LVGL's decoder reads baseline "
         "only. Save it as a baseline JPEG.");
  return {big16(d, frame + 7), big16(d, frame + 5)};
}

consteval picture_size png_size(file_bytes d) {
  return {big16(d, 16) << 16 | big16(d, 18), big16(d, 20) << 16 | big16(d, 22)};
}

consteval picture_size checked_size(picture_size size) {
  if (size.w == 0 || size.h == 0 || size.w > 0xffff || size.h > 0xffff)
    fail("lv: this picture is " + decimal(size.w) + " x " + decimal(size.h) +
         " pixels. LVGL holds a picture of 1 to 65535 pixels each way.");
  return size;
}

template <class D>
consteval void needs_decoder(bool png) {
  if (png && png_decoder<D> == std::meta::info{})
    fail("lv: lv::encoded_image keeps this PNG as a file, and LVGL is built without its PNG decoder. Turn it on in "
         "CMake, before lvgl26 is added: set(LVGL26_PNG ON). Or let the compiler decode the picture: lv::image_file.");
  if (!png && jpeg_decoder<D> == std::meta::info{})
    fail("lv: lv::encoded_image keeps this JPEG as a file, and LVGL is built without its JPEG decoder. Turn it on in "
         "CMake, before lvgl26 is added: set(LVGL26_JPEG ON).");
}

// The file `file` as an LVGL image whose data is the file itself.
template <class D>
consteval const lv_image_dsc_t* make_encoded(file_bytes file) {
  const bool png = is_png(file);
  if (!png && !is_jpeg(file))
    fail("lv: lv::encoded_image takes a PNG or a JPEG file, and this is neither. Embed one: "
         "`constexpr unsigned char photo_jpg[] = { #embed \"photo.jpg\" };`");
  needs_decoder<D>(png);
  const picture_size size = checked_size(png ? png_size(file) : jpeg_size(file));
  lv_image_dsc_t dsc{};
  dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  dsc.header.cf = static_cast<std::uint32_t>(png ? LV_COLOR_FORMAT_RAW_ALPHA : LV_COLOR_FORMAT_RAW) & 0xff;
  dsc.header.w = size.w & 0xffff, dsc.header.h = size.h & 0xffff;
  dsc.data_size = static_cast<std::uint32_t>(file.size());
  dsc.data = file.data();  // the program's own array: nothing is copied
  return std::define_static_array(std::vector{dsc}).data();
}

template <const auto& File, class D = void>
inline constexpr const lv_image_dsc_t* encoded_of = make_encoded<D>(File);

}  // namespace detail

// The embedded PNG or JPEG file `File`, kept as it is and decoded on the device:
// what an image widget takes as its `.src`.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* encoded_image = detail::encoded_of<File>;

}  // namespace lv
