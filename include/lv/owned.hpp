#pragma once

#include <lv/view.hpp>

#include <utility>

namespace lv {

template <detail::a_view V>
class owned;

// A view that empties itself when LVGL deletes the widget.
// Use it for a widget that something else may delete, such as its parent.
template <detail::a_view V>
class weak {
  lv_obj_t* p_ = nullptr;
  lv_delete_dsc_t* dsc_ = nullptr;
  friend owned<V>;

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

  // Takes over what `o` tracks. Lets go of the other's callback first, so that the
  // new one can use its memory.
  void take(weak& o) {
    lv_obj_t* p = o.p_;
    o.detach();
    attach(p);
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
  weak(weak&& o) noexcept { take(o); }
  weak& operator=(const weak& o) {
    if (this != &o) {
      detach();
      attach(o.p_);
    }
    return *this;
  }
  weak& operator=(weak&& o) noexcept {
    if (this != &o) {
      detach();
      take(o);
    }
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
// An owner that LVGL has no memory to track its widget with deletes the widget at
// once and is empty: check it with operator bool.
template <detail::a_view V>
class owned {
  weak<V> w_;

  // Owns `v`, or deletes it if it cannot be tracked.
  void adopt(V v) {
    w_.detach();
    w_.attach(v.raw());
    if (v && !w_) lv_obj_delete(v.raw());
  }

 public:
  owned() = default;
  explicit owned(V v) { adopt(v); }
  owned(owned&& o) noexcept { adopt(o.release()); }
  owned& operator=(owned&& o) noexcept {
    if (this != &o) {
      reset();
      adopt(o.release());
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
