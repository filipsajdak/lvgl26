#pragma once

// Every style lvgl26 adds to a widget goes through add_style: the start of a factory, a node's own
// style, a look, a layer of lv::on, a theme, the rows and columns of a document.
//
// lv_obj_add_style of LVGL v9.6.0 and of the v10 pin grows the widget's list of styles with an
// unchecked lv_realloc, and writes through the null pointer when it fails (lvgl/lvgl#11014). A
// later LVGL returns lv_result_t instead (lvgl/lvgl#11015). add_style uses that result where the
// function returns one; otherwise it first makes sure that a list of the most styles LVGL allows
// can be allocated, and adds nothing when it cannot.

#include <lvgl/lvgl.h>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace lv::detail {

// LVGL keeps the number of a widget's styles in 6 bits and refuses the 64th (lv_obj_style.c).
inline constexpr std::size_t most_styles_of_a_widget = 63;

// One entry of the list (lv_obj_style_t, private) is a style pointer and a 32-bit word of
// selector and flags: at most the size of two pointers on every ABI LVGL supports.
inline constexpr std::size_t pointers_in_a_style_entry = 2;
inline constexpr std::size_t style_entry_bound = pointers_in_a_style_entry * sizeof(void*);

// The most bytes that the list of a widget's styles can grow to: 1008 on a 64-bit target, 504 on
// a 32-bit one. tests/style_add.cpp pins both numbers by measurement.
inline constexpr std::size_t style_list_bound = most_styles_of_a_widget * style_entry_bound;

// Whether lv_realloc can grow a widget's list of styles by one entry. A block of the bound is taken
// and given back: with one thread on LVGL's heap, the realloc that follows then finds room.
inline bool room_for_a_style() {
  void* probe = lv_malloc(style_list_bound);
  if (probe == nullptr) return false;
  lv_free(probe);
  return true;
}

// An add function that reports its failure, as lv_obj_add_style does from lvgl/lvgl#11015 on.
template <auto Add>
concept reports_failure =
    std::same_as<std::invoke_result_t<decltype(Add), lv_obj_t*, const lv_style_t*, lv_style_selector_t>, lv_result_t>;

template <auto Add>
  requires reports_failure<Add>
bool add_style_with(lv_obj_t* o, const lv_style_t* style, lv_style_selector_t selector) {
  return Add(o, style, selector) == LV_RESULT_OK;
}

template <auto Add>
bool add_style_with(lv_obj_t* o, const lv_style_t* style, lv_style_selector_t selector) {
  if (!room_for_a_style()) return false;
  Add(o, style, selector);
  return true;
}

// Adds `style` to `o` as lv_obj_add_style does. False when LVGL's heap has no room for it: the
// widget is then as it was.
inline bool add_style(lv_obj_t* o, const lv_style_t* style, lv_style_selector_t selector) {
  return add_style_with<&lv_obj_add_style>(o, style, selector);
}

}  // namespace lv::detail
