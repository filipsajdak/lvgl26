#pragma once

// The precompiled header of the tests when LVGL26_WARM_PCH is on (cmake/warm_pch.cmake): the
// library, and the work that every file with a widget does first, done once. Each mount below
// makes the compiler scan LVGL's API, the styles and the members of a widget kind; the header
// keeps the results, so a test that includes it does not scan again.
#include <lv/lv.hpp>

namespace lv_warm_pch {

constexpr auto box = lv::box();
constexpr auto label = lv::label({.text = "warm"});
constexpr auto button = lv::button();

inline void warm(lv::obj screen) {
  auto a = lv::mount<box>(screen);
  auto b = lv::mount<label>(screen);
  auto c = lv::mount<button>(screen);
  (void)a;
  (void)b;
  (void)c;
}

}  // namespace lv_warm_pch
