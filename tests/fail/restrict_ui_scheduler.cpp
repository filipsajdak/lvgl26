// A part of lvgl26 that needs threads, in a project that does not allow it.
#include <lv/config.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.allow = {.threads = false}};

#include <lv/lv.hpp>
#include <lv/ui_scheduler.hpp>

void use() {
  lv::ui_scheduler ui;
}
