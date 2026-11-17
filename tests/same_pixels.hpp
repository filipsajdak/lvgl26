#pragma once

// Compares a port of an LVGL example with LVGL's C original, pixel for pixel.
// Both are built on a fresh screen, the same steps are applied to both (a click,
// a new value, some time), and a picture is taken of each.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstring>

// A picture of a screen, owned.
class picture {
  lv_draw_buf_t* buf_;

 public:
  explicit picture(lv_draw_buf_t* buf) : buf_(buf) {}
  picture(picture&& other) noexcept : buf_(other.buf_) { other.buf_ = nullptr; }
  picture(const picture&) = delete;
  picture& operator=(const picture&) = delete;
  ~picture() {
    if (buf_) lv_draw_buf_destroy(buf_);
  }

  bool valid() const { return buf_ != nullptr; }

  // Where two pictures differ: the box around the differing pixels, and how many.
  void describe_difference(const picture& other) const {
    long x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1, n = 0;
    const long w = buf_->header.w, h = buf_->header.h;
    for (long y = 0; y < h; ++y)
      for (long x = 0; x < w; ++x) {
        const auto i = static_cast<std::size_t>(y) * buf_->header.stride + static_cast<std::size_t>(x) * 4;
        if (std::memcmp(buf_->data + i, other.buf_->data + i, 4) != 0) {
          ++n;
          x0 = x < x0 ? x : x0; y0 = y < y0 ? y : y0;
          x1 = x > x1 ? x : x1; y1 = y > y1 ? y : y1;
        }
      }
    std::printf("  %ld pixels differ, inside x %ld..%ld, y %ld..%ld\n", n, x0, x1, y0, y1);
  }

  // The first pixel where two pictures differ, or -1 when they are the same.
  long first_difference(const picture& other) const {
    if (!buf_ || !other.buf_) return 0;
    if (buf_->header.w != other.buf_->header.w || buf_->header.h != other.buf_->header.h) return 0;
    const auto size = static_cast<std::size_t>(buf_->header.stride) * buf_->header.h;
    for (std::size_t i = 0; i < size; ++i)
      if (buf_->data[i] != other.buf_->data[i]) return static_cast<long>(i / 4);
    return -1;
  }
};

// Runs `build` on a fresh screen, then `act`, and takes a picture.
template <class Build, class Act>
picture render(Build build, Act act) {
  lv_obj_t* previous = lv_screen_active();
  lv_obj_t* screen = lv_obj_create(nullptr);
  lv_screen_load(screen);
  build(lv::obj{screen});
  act(lv::obj{screen});
  picture result{lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888)};
  lv_screen_load(previous);
  lv_obj_delete(screen);
  return result;
}

inline void nothing(lv::obj) {}

// The widget number `index` of class `kind` under `root`, counted depth first, or null.
inline lv_obj_t* find_widget(lv_obj_t* root, const lv_obj_class_t* kind, int& index) {
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
    lv_obj_t* child = lv_obj_get_child(root, static_cast<std::int32_t>(i));
    if (lv_obj_check_type(child, kind) && index-- == 0) return child;
    if (lv_obj_t* found = find_widget(child, kind, index)) return found;
  }
  return nullptr;
}

inline lv_obj_t* nth(lv::obj root, const lv_obj_class_t* kind, int index = 0) {
  lv_obj_t* found = find_widget(root.raw(), kind, index);
  check(found != nullptr);
  return found;
}

// Clicks the first button found under `root`, searching depth first.
inline void click_first_button(lv::obj root) {
  lv_obj_send_event(nth(root, &lv_button_class), LV_EVENT_CLICKED, nullptr);
}

// Lets `ms` milliseconds pass, running LVGL's timers and animations every 10 ms.
inline void pass_time(std::uint32_t ms) {
  for (std::uint32_t t = 0; t < ms; t += 10) {
    lv_tick_inc(10);
    lv_timer_handler();
  }
}

// Builds the C original and the port, applies `act` to each, and checks that the
// two pictures are the same.
template <class Port, class Act>
void same_pixels(const char* name, void (*original)(), Port port, Act act) {
  const auto c_version = render([&](lv::obj) { original(); }, act);
  const auto ported = render(port, act);
  const long at = c_version.first_difference(ported);
  check(c_version.valid() && ported.valid());
  check(at == -1);
  std::printf("%-28s %s", name, at == -1 ? "same pixels\n" : "DIFFERENT");
  if (at != -1) {
    std::printf(" at pixel %ld (x=%ld, y=%ld)\n", at, at % 320, at / 320);
    c_version.describe_difference(ported);
  }
}
