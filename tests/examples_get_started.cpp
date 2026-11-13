// Each get_started example, rewritten with lvgl26, must render the same pixels
// as LVGL's C original.
//
// This test also guards a linker trap: the four ports each have a tree called `ui`
// in an unnamed namespace. If the code generated for one were shared with another,
// the pictures would show the wrong widgets.

#include "../examples/lvgl/get_started/get_started.hpp"

#include "check.hpp"

#include <cstring>

extern "C" {
void lv_example_get_started_hello_world(void);
void lv_example_get_started_slider(void);
void lv_example_get_started_styles(void);
void lv_example_get_started_button(void);
}

namespace {

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

void nothing(lv::obj) {}

// Clicks the first button found under `root`, searching depth first.
void click_first_button(lv::obj root) {
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(root.raw()); ++i) {
    lv_obj_t* child = lv_obj_get_child(root.raw(), static_cast<std::int32_t>(i));
    if (lv_obj_check_type(child, &lv_button_class)) {
      lv_obj_send_event(child, LV_EVENT_CLICKED, nullptr);
      return;
    }
    click_first_button(lv::obj{child});
  }
}

template <class Port, class Act>
void same_pixels(const char* name, void (*original)(), Port port, Act act) {
  const auto c_version = render([&](lv::obj) { original(); }, act);
  const auto ported = render(port, act);
  const long at = c_version.first_difference(ported);
  check(c_version.valid() && ported.valid());
  check(at == -1);
  std::printf("%-12s %s", name, at == -1 ? "same pixels\n" : "DIFFERENT");
  if (at != -1) {
    std::printf(" at pixel %ld (x=%ld, y=%ld)\n", at, at % 320, at / 320);
    c_version.describe_difference(ported);
  }
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);

  same_pixels("hello_world", &lv_example_get_started_hello_world, &get_started::hello_world, &nothing);
  same_pixels("slider", &lv_example_get_started_slider, &get_started::slider, &nothing);
  same_pixels("styles", &lv_example_get_started_styles, &get_started::styles, &nothing);
  // The two differ before the first click (see button.cpp), so compare after one.
  same_pixels("button", &lv_example_get_started_button, &get_started::button, &click_first_button);

  // A different picture must be noticed.
  const auto hello = render(&get_started::hello_world, &nothing);
  const auto styled = render(&get_started::styles, &nothing);
  check(hello.first_difference(styled) != -1);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
