#pragma once

// Editor stub of lv/owned.hpp.

#include <lv/view.hpp>

namespace lv {

/// A view that empties itself when LVGL deletes the widget.
/// Use it for a widget that something else may delete, such as its parent.
template <view_value V>
class weak {
 public:
  weak() = default;
  explicit weak(V v);
  weak(const weak& o);
  weak(weak&& o) noexcept;
  weak& operator=(weak o) noexcept;
  ~weak();

  /// The widget, or an empty view once it was deleted.
  V get() const;
  /// True once the widget was deleted.
  bool expired() const;
  explicit operator bool() const;
};

/// Owns a widget: deletes it when the owner goes away.
/// Safe when the parent deletes the widget first.
template <view_value V>
class owned {
 public:
  owned() = default;
  explicit owned(V v);
  owned(owned&&) noexcept;
  owned& operator=(owned&& o) noexcept;
  ~owned();

  /// Deletes the widget now, if it still exists.
  void reset();
  /// Gives the widget back to its parent. Nothing is deleted.
  V release();
  /// The widget, or an empty view once it was deleted.
  V get() const;
  explicit operator bool() const;
};

}  // namespace lv
