#pragma once

// Editor stub of lv/event.hpp.

#include <lvgl/lvgl.h>

#include <concepts>

namespace lv {

/// What an event handler receives.
class event {
  lv_event_t* e_;

 public:
  constexpr explicit event(lv_event_t* e) : e_(e) {}
  /// LVGL's event, for code that still calls the C API.
  constexpr lv_event_t* raw() const { return e_; }
  /// LVGL's code of the event.
  lv_event_code_t code() const;

  /// The widget the handler was added to, as the view type you ask for.
  template <class V>
    requires std::constructible_from<V, lv_obj_t*>
  V target() const;
};

}  // namespace lv
