#pragma once

#include <lvgl/lvgl.h>

#include <concepts>

namespace lv {

// What an event handler receives.
class event {
  lv_event_t* e_;

 public:
  constexpr explicit event(lv_event_t* e) : e_(e) {}
  constexpr lv_event_t* raw() const { return e_; }
  lv_event_code_t code() const { return lv_event_get_code(e_); }

  // The widget the handler was added to, as the view type you ask for.
  template <class V>
    requires std::constructible_from<V, lv_obj_t*>
  V target() const {
    return V{lv_event_get_current_target_obj(e_)};
  }
};

}  // namespace lv
