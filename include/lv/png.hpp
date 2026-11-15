#pragma once

// A widget, or a whole screen, as a PNG picture, without a display to look at.
// Hosted: it allocates, and write_png writes a file.
//
//   auto file = lv::write_png(lv::screen_active(), "screen.png");   // std::expected
//   auto bytes = lv::encode_png(lv::screen_active());               // the file in memory
//
// LVGL must be built with LV_USE_SNAPSHOT. The picture is 8-bit RGBA, drawn by
// lv_snapshot_take: what the widget and its children look like now, whether or
// not a display has shown them yet. The PNG is written by lv/detail/png.hpp,
// because LVGL's public API reads PNG files but does not write them.

#include <lv/detail/png.hpp>
#include <lv/view.hpp>

#include <cstdint>
#include <cstdio>
#include <expected>
#include <vector>

namespace lv {

enum class png_error {
  no_picture,    // LVGL could not draw the widget: it has no size, or memory ran out
  cannot_write,  // the file could not be opened or written
};

// The widget `what` and its children as the bytes of a PNG file.
inline std::expected<std::vector<std::uint8_t>, png_error> encode_png(obj what)
  pre(what.raw() != nullptr)
{
  lv_draw_buf_t* picture = lv_snapshot_take(what.raw(), LV_COLOR_FORMAT_ARGB8888);
  if (picture == nullptr) return std::unexpected(png_error::no_picture);
  auto file = detail::png::encode(picture->data, picture->header.w, picture->header.h,
                                  picture->header.stride);
  lv_draw_buf_destroy(picture);
  return file;
}

// Writes the widget `what` and its children to a PNG file at `path`.
inline std::expected<void, png_error> write_png(obj what, const char* path)
  pre(what.raw() != nullptr)
  pre(path != nullptr)
{
  const auto file = encode_png(what);
  if (!file) return std::unexpected(file.error());
  std::FILE* out = std::fopen(path, "wb");
  if (out == nullptr) return std::unexpected(png_error::cannot_write);
  const bool written = std::fwrite(file->data(), 1, file->size(), out) == file->size();
  const bool closed = std::fclose(out) == 0;
  if (!written || !closed) return std::unexpected(png_error::cannot_write);
  return {};
}

}  // namespace lv
