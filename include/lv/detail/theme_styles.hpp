#pragma once

// What lv::theme makes and lv::display takes. lv/theme.hpp builds it; this header is
// all that a display needs to know.

#include <lvgl/lvgl.h>

#include <cstddef>

namespace lv {

class theme_styles;

namespace detail {

// One style of a theme: for which widgets, for which part or state.
struct theme_entry {
  const lv_obj_class_t* widget;  // null: a screen
  lv_style_selector_t selector;
  const lv_style_t* style;  // null: the end of the theme
};

// LVGL calls this for every new widget.
inline void apply_theme(lv_theme_t* theme, lv_obj_t* o) {
  const lv_obj_class_t* widget = lv_obj_get_parent(o) != nullptr ? lv_obj_get_class(o) : nullptr;
  for (auto e = static_cast<const theme_entry*>(lv_theme_get_user_data(theme)); e->style != nullptr; ++e)
    if (e->widget == widget) lv_obj_add_style(o, e->style, e->selector);
}

struct theme_access;

}  // namespace detail

// What lv::theme returns: a view of the theme's styles in read-only memory. Give it
// to an lv::display.
class theme_styles {
 public:
  constexpr theme_styles() = default;

  // True for a theme that sets nothing.
  constexpr bool empty() const { return count_ == 0; }

 private:
  friend struct detail::theme_access;

  constexpr theme_styles(const detail::theme_entry* first, std::size_t count) : first_(first), count_(count) {}

  const detail::theme_entry* first_ = nullptr;
  // A number, not a look at the pointer: under a sanitizer GCC 16.2 does not treat a
  // null check of a static address as a constant.
  std::size_t count_ = 0;
};

namespace detail {

struct theme_access {
  // `count` styles, then the entry that ends the list.
  static constexpr theme_styles make(const theme_entry* first, std::size_t count) { return {first, count}; }
  static constexpr const theme_entry* first(theme_styles t) { return t.first_; }
};

}  // namespace detail

}  // namespace lv
