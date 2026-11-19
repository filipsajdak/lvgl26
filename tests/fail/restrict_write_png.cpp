// A part of lvgl26 that needs files, in a project that does not allow it.
#include <lv/config.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.allow = {.files = false}};

#include <lv/lv.hpp>
#include <lv/png.hpp>

void use() {
  static_cast<void>(lv::write_png(lv::screen_active(), "screen.png"));
}
