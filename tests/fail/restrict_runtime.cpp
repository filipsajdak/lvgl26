// A part of lvgl26 that needs floating point, in a project that does not allow it.
#include <lv/config.hpp>
#include <lv/runtime.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.allow = {.floating_point = false}};

#include <lv/lv.hpp>

void use() {
  static_cast<void>(lv::runtime<"obj", "label">::load("{}", lv::screen_active()));
}
