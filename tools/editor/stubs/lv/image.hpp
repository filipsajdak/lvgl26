#pragma once

// Editor stub of lv/image.hpp.

#include <lv/detail/missing.hpp>
#include <lvgl/lvgl.h>

#include <lv/config.hpp>
#include <lv/detail/embedded.hpp>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/image.hpp"> = true;
/// The alpha-only picture of an embedded PNG file, for a style to tint with image_recolor:
/// the alpha of the file when some pixel is see-through, otherwise the grey value of each pixel.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)> && detail::included<"lv/image.hpp">
inline constexpr const lv_image_dsc_t* mask_file<File> = nullptr;

}  // namespace lv::detail

namespace lv {

/// The picture of an embedded PNG file, decoded by the compiler into the display's
/// own pixels: lv::image({.src = lv::image_file<logo_png>}).
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)> && detail::included<"lv/image.hpp">
inline constexpr const lv_image_dsc_t* image_file<File> = nullptr;

}  // namespace lv
