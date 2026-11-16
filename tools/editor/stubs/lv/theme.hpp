#pragma once

// Editor stub of lv/theme.hpp. The members of a theme are in lv/generated.hpp.

#include <lv/detail/theme_styles.hpp>
#include <lv/generated.hpp>
#include <lv/props.hpp>
#include <lv/style.hpp>

namespace lv {

/// A theme: the styles that every widget of a kind starts with. One member for each
/// LVGL widget (`arc`, `button`, `obj` for a plain box, `toggle` for the switch) and
/// `screen`, in alphabetical order. Each holds `style`, `style_pressed`, and the rest.
consteval theme_styles theme(const stub::theme_spec&) {
  return stub::theme_access::some();
}

}  // namespace lv
