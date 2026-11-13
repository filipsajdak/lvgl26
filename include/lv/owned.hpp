#pragma once

#include <lv/view.hpp>

#include <utility>

namespace lv {

// A view that empties itself when LVGL deletes the widget.
// Use it for a widget that something else may delete, such as its parent.
template <detail::a_view V>
class weak {
  lv_obj_t* p_ = nullptr;
  lv_delete_dsc_t* dsc_ = nullptr;

  static void on_delete(void* self) {
    auto* w = static_cast<weak*>(self);
    w->p_ = nullptr;
    w->dsc_ = nullptr;
  }

  void attach(lv_obj_t* p) {
    dsc_ = p ? lv_obj_add_delete_cb(p, &on_delete, this) : nullptr;
    // Without the callback the widget cannot be tracked, so report it as gone.
    p_ = dsc_ ? p : nullptr;
  }

  void detach() {
    if (dsc_) lv_obj_remove_delete_cb(dsc_);
    p_ = nullptr;
    dsc_ = nullptr;
  }

 public:
  weak() = default;
  explicit weak(V v) { attach(v.raw()); }
  weak(const weak& o) { attach(o.p_); }
  weak(weak&& o) noexcept {
    attach(o.p_);
    o.detach();
  }
  weak& operator=(weak o) noexcept {
    detach();
    attach(o.p_);
    return *this;
  }
  ~weak() { detach(); }

  // The widget, or an empty view once it was deleted.
  V get() const { return V{p_}; }
  bool expired() const { return p_ == nullptr; }
  explicit operator bool() const { return p_ != nullptr; }
};

// Owns a widget: deletes it when the owner goes away.
// Safe when the parent deletes the widget first.
template <detail::a_view V>
class owned {
  weak<V> w_;

 public:
  owned() = default;
  explicit owned(V v) : w_(v) {}
  owned(owned&&) noexcept = default;
  owned& operator=(owned&& o) noexcept {
    if (this != &o) {
      reset();
      w_ = std::move(o.w_);
    }
    return *this;
  }
  ~owned() { reset(); }

  // Deletes the widget now, if it still exists.
  void reset() {
    if (V v = w_.get()) lv_obj_delete(v.raw());
  }

  // Gives the widget back to its parent. Nothing is deleted.
  V release() {
    V v = w_.get();
    w_ = weak<V>{};
    return v;
  }

  V get() const { return w_.get(); }
  explicit operator bool() const { return static_cast<bool>(w_); }
};

}  // namespace lv
