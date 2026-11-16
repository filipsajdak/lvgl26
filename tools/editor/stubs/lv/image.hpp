#pragma once

// Editor stub of lv/image.hpp.

#include <lvgl/lvgl.h>

#include <lv/config.hpp>
#include <lv/detail/embedded.hpp>

namespace lv {

/// The picture of an embedded PNG file, decoded by the compiler into the display's
/// own pixels: lv::image({.src = lv::image_file<logo_png>}).
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* image_file = nullptr;

}  // namespace lv
