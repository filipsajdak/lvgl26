#pragma once

// Editor stub of lv/png.hpp: a widget, or a whole screen, as a PNG picture.

#include <lv/view.hpp>

#include <cstdint>
#include <expected>
#include <vector>

namespace lv {

/// Why a picture could not be made or written.
enum class png_error {
  no_picture,    // LVGL could not draw the widget: it has no size, or memory ran out
  cannot_write,  // the file could not be opened or written
  file_exists,   // write_new_png: there is a file, or a link, of that name already
};

/// The widget `what` and its children as the bytes of a PNG file.
std::expected<std::vector<std::uint8_t>, png_error> encode_png(obj what);

/// Writes the widget `what` and its children to a PNG file at `path`. A file that
/// is there already is replaced.
std::expected<void, png_error> write_png(obj what, const char* path);

/// The same, for a name that comes from outside: it only ever makes a new file.
std::expected<void, png_error> write_new_png(obj what, const char* path);

}  // namespace lv
