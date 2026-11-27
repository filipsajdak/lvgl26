#pragma once

// What a mode look (lv/mode_look.hpp) is at run time: a constant table of the styles of the look in each
// mode, and the walk that puts the styles of another mode in their place when a display that follows a
// state changes it. Nothing here is a template and nothing allocates but the marker that join_mode_look
// adds to a widget (one event descriptor, as a handler is).
//
// A widget that took `.look = lv::mode_look<"tile">` carries a marker: an event of LVGL's whose callback
// does nothing and whose user data is the table. The walk of a display finds the marker of each widget,
// so a look in a group, in a row of lv::each and at any depth is found, and a style of the program that
// happens to be the same constant is never taken for a mode look's.

#include <lv/detail/style_add.hpp>

#include <lvgl/lvgl.h>

#include <cstddef>

namespace lv::detail {

// One style of a look in one mode: the look's own style (the main part, the default state) and then each
// layer that lv::on or a nested group made, with its selector.
struct mode_entry {
  const lv_style_t* style = nullptr;
  lv_style_selector_t selector = 0;
};

// The styles of one look in every mode: `modes` rows of `per` entries, the same selectors in each row.
struct mode_object {
  const mode_entry* entries = nullptr;
  std::size_t modes = 0;
  std::size_t per = 0;

  constexpr const mode_entry& at(std::size_t mode, std::size_t k) const {
    return entries[(mode < modes ? mode : modes - 1) * per + k];
  }
};

// What `.look` holds: the table of one look.
struct mode_look_ref {
  const mode_object* object;  // no default member initializer: an unset .look is all zeros
};

// The mode that new widgets are built in: the index of the value of the state that a display follows.
inline std::size_t mode_now = 0;

inline void mode_marker(lv_event_t*) {}

// Gives `o` the styles of the look in the mode of the moment, and the marker that a switch finds. False
// when LVGL's heap had no room for one of them.
inline bool join_mode_look(lv_obj_t* o, mode_look_ref look) {
  const mode_object& m = *look.object;
  bool all = true;
  for (std::size_t k = 0; k < m.per; ++k) all = add_style(o, m.at(mode_now, k).style, m.at(mode_now, k).selector) && all;
  return lv_obj_add_event_cb(o, &mode_marker, LV_EVENT_DELETE, const_cast<mode_object*>(&m)) != nullptr && all;
}

// Puts the styles of mode `to` where those of mode `from` stand, in `o` and below it. A style takes the
// place of the other in the widget's list, so what was added after it still wins over it.
inline void switch_mode_in(lv_obj_t* o, std::size_t from, std::size_t to) {
  for (std::uint32_t i = 0, n = lv_obj_get_event_count(o); i < n; ++i) {
    lv_event_dsc_t* dsc = lv_obj_get_event_dsc(o, i);
    if (lv_event_dsc_get_cb(dsc) != &mode_marker) continue;
    const auto& m = *static_cast<const mode_object*>(lv_event_dsc_get_user_data(dsc));
    for (std::size_t k = 0; k < m.per; ++k)
      if (m.at(from, k).style != m.at(to, k).style)
        static_cast<void>(lv_obj_replace_style(o, m.at(from, k).style, m.at(to, k).style, m.at(from, k).selector));
  }
  for (std::uint32_t i = 0, n = lv_obj_get_child_count(o); i < n; ++i)
    switch_mode_in(lv_obj_get_child(o, static_cast<int32_t>(i)), from, to);
}

}  // namespace lv::detail
