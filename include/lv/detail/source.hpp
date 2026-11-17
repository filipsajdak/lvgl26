#pragma once

// What a binding needs to know about the state or computed it follows, without knowing
// its type: its watchers, its value, and where it can be written. One table for each type
// (source_ops_of<S>, in lv/state.hpp), reached through one function pointer, so that the
// property of a node holds a single pointer for all of it.

#include <lv/detail/watch.hpp>

#include <lvgl.h>

#include <cstdint>

namespace lv::detail {

// The value of a state in the form an LVGL subject holds it: a number for everything but
// a color and a pointer.
struct subject_value {
  std::int32_t number;
  lv_color_t color;
  const void* pointer;
};

struct source_ops {
  // The LVGL subject, created on first use. Null only if LVGL has no memory left.
  lv_subject_t* (*subject)(const void* source);
  // The watchers. A computed starts to follow its own dependencies when asked.
  watch_list& (*watchers)(const void* source);
  // The current value.
  subject_value (*read)(const void* source);
  // Sets the value from a number a widget gave back. Null for what only follows.
  void (*write)(const void* source, std::int32_t number);
};

using source_ops_fn = const source_ops& (*)();

}  // namespace lv::detail
