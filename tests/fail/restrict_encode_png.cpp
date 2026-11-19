// A part of lvgl26 that needs allocation, in a project that does not allow it.
#include <lv/config.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.allow = {.allocation = false}};

#include <lv/lv.hpp>
#include <lv/png.hpp>

void use() {
  static_cast<void>(lv::encode_png(lv::screen_active()));
}
