#pragma once

// Editor stub of lv/encoded_image.hpp.

#include <lvgl/lvgl.h>

#include <lv/detail/embedded.hpp>

namespace lv {

/// An embedded PNG or JPEG file kept as it is, and decoded on the device by LVGL:
/// lv::image({.src = lv::encoded_image<photo_jpg>}). Needs LVGL26_PNG or LVGL26_JPEG.
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* encoded_image = nullptr;

}  // namespace lv
