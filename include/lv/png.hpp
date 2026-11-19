#pragma once

// A widget, or a whole screen, as a PNG picture, without a display to look at.
// Hosted: it allocates, and write_png writes a file.
//
//   auto file = lv::write_png(lv::screen_active(), "screen.png");   // std::expected
//   auto once = lv::write_new_png(lv::screen_active(), "screen.png");  // never replaces a file
//   auto bytes = lv::encode_png(lv::screen_active());               // the file in memory
//
// LVGL must be built with LV_USE_SNAPSHOT. The picture is 8-bit RGBA, drawn by
// lv_snapshot_take: what the widget and its children look like now, whether or
// not a display has shown them yet. The PNG is written by lv/detail/png.hpp,
// because LVGL's public API reads PNG files but does not write them.

#include <lv/detail/png.hpp>
#include <lv/detail/settings.hpp>
#include <lv/view.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <vector>

namespace lv {

enum class png_error {
  no_picture,    // LVGL could not draw the widget: it has no size, or memory ran out
  cannot_write,  // the file could not be opened or written
  file_exists,   // write_new_png: there is a file, or a link, of that name already
};

// The widget `what` and its children as the bytes of a PNG file.
// O only delays reading the project's settings until the function is used.
template <class O = override>
std::expected<std::vector<std::uint8_t>, png_error> encode_png(obj what)
  pre(what.raw() != nullptr)
{
  detail::require<^^allow_settings::allocation, O, "lv::encode_png", " The bytes of the file are a std::vector.">();
  lv_draw_buf_t* picture = lv_snapshot_take(what.raw(), LV_COLOR_FORMAT_ARGB8888);
  if (picture == nullptr) return std::unexpected(png_error::no_picture);
  auto file = detail::png::encode(picture->data, picture->header.w, picture->header.h,
                                  picture->header.stride);
  lv_draw_buf_destroy(picture);
  return file;
}

namespace detail {

// Why a file could not be opened for writing.
inline png_error open_error(int error) {
  return error == EEXIST ? png_error::file_exists : png_error::cannot_write;
}

// Draws `what` and writes it to `path`, opened with the fopen mode `mode`.
template <class O>
std::expected<void, png_error> write_png_file(obj what, const char* path, const char* mode) {
  require<^^allow_settings::files, O, "lv::write_png">();
  const auto file = encode_png<O>(what);
  if (!file) return std::unexpected(file.error());
  errno = 0;
  std::FILE* out = std::fopen(path, mode);
  if (out == nullptr) return std::unexpected(open_error(errno));
  const bool written = std::fwrite(file->data(), 1, file->size(), out) == file->size();
  const bool closed = std::fclose(out) == 0;
  if (!written || !closed) return std::unexpected(png_error::cannot_write);
  return {};
}

}  // namespace detail

// Writes the widget `what` and its children to a PNG file at `path`. A file that
// is there already is replaced.
template <class O = override>
std::expected<void, png_error> write_png(obj what, const char* path)
  pre(what.raw() != nullptr)
  pre(path != nullptr)
{
  return detail::write_png_file<O>(what, path, "wb");
}

// The same, for a name that comes from outside: it only ever makes a new file.
// If `path` names a file or a symbolic link, nothing is written (fopen mode "x").
template <class O = override>
std::expected<void, png_error> write_new_png(obj what, const char* path)
  pre(what.raw() != nullptr)
  pre(path != nullptr)
{
  return detail::write_png_file<O>(what, path, "wbx");
}

}  // namespace lv
