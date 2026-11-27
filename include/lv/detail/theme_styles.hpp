#pragma once

// What lv::theme makes and lv::display takes. lv/theme.hpp builds it; this header is
// all that a display needs to know.

#include <lv/detail/style_add.hpp>
#include <lvgl/lvgl.h>

#include <cstddef>
#include <cstdint>

namespace lv {

class theme_styles;

namespace detail {

// One style of a theme: for which widgets, for which part or state.
struct theme_entry {
  const lv_obj_class_t* widget;  // null: a screen
  lv_style_selector_t selector;
  const lv_style_t* style;  // null: the end of the theme
};

// LVGL calls this for every new widget, inside its creation, so a style without room in LVGL's
// heap is left out rather than failing the widget: the next allocation of the mount fails it.
inline void apply_theme(lv_theme_t* theme, lv_obj_t* o) {
  const lv_obj_class_t* widget = lv_obj_get_parent(o) != nullptr ? lv_obj_get_class(o) : nullptr;
  for (auto e = static_cast<const theme_entry*>(lv_theme_get_user_data(theme)); e->style != nullptr; ++e)
    if (e->widget == widget) static_cast<void>(add_style(o, e->style, e->selector));
}

// The entry that ends a theme's list.
inline const theme_entry* theme_end(const theme_entry* first) {
  while (first->style != nullptr) ++first;
  return first;
}

// Puts the styles of the theme `to` where the theme `from` put its own, in this widget and in all it
// holds. Entry i of `to` takes the place of entry i of `from`: LVGL swaps the style where it stands in the
// widget's list, so a look or a node's style that was added after the theme still wins over it, and nothing
// is allocated. A widget that holds no style of `from` (a group removed them all) is left as it is.
// Both lists have the same shape: the same class and selector in each position.
inline void restyle(lv_obj_t* o, const theme_entry* from, const theme_entry* to) {
  const lv_obj_class_t* widget = lv_obj_get_parent(o) != nullptr ? lv_obj_get_class(o) : nullptr;
  for (const theme_entry *a = from, *b = to; a->style != nullptr; ++a, ++b)
    if (a->widget == widget && a->style != b->style) static_cast<void>(lv_obj_replace_style(o, a->style, b->style, a->selector));
  for (std::uint32_t i = 0, n = lv_obj_get_child_count(o); i < n; ++i) restyle(lv_obj_get_child(o, static_cast<int32_t>(i)), from, to);
}

// How many themes a display follows at most.
inline constexpr std::size_t most_followed_themes = 8;

// Whether two lists of entries have the same shape: the same class and selector in each position.
inline bool same_shape(const theme_entry* a, const theme_entry* b) {
  for (; a->style != nullptr && b->style != nullptr; ++a, ++b)
    if (a->widget != b->widget || a->selector != b->selector) return false;
  return a->style == nullptr && b->style == nullptr;
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
  // The entries of a theme, ending with the entry that has no style: a theme that sets nothing has one entry, the end.
  static constexpr const theme_entry* entries(theme_styles t) { return t.first_ != nullptr ? t.first_ : &none; }
  static constexpr theme_entry none{nullptr, 0, nullptr};
};

}  // namespace detail

}  // namespace lv
