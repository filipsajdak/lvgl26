// lv/signal_gesture.hpp: Signal's gesture limits, from include/lv/signal/gesture.json, and a pointer that takes them.
#include <lv/gesture.hpp>
#include <lv/lv.hpp>
#include <lv/pointer.hpp>
#include <lv/signal_gesture.hpp>

#include "check.hpp"

#include <type_traits>

namespace {

using namespace lv::units;

// The values of design/signal/tokens.design.json, section "gesture": LVGL's own defaults.
static_assert(lv::signal::gesture.swipe_distance == 50 * px);
static_assert(lv::signal::gesture.swipe_speed == 3 * px);
static_assert(lv::signal::gesture.scroll_limit == 10 * px);
static_assert(lv::signal::gesture.long_press == 400 * ms);

static_assert(std::is_same_v<decltype(lv::signal::gesture.swipe_distance), lv::token<lv::length>>);
static_assert(std::is_same_v<decltype(lv::signal::gesture.long_press), lv::token<lv::duration>>);

// The group is what a pointer takes.
static_assert(lv::gesture_settings<std::remove_cvref_t<decltype(lv::signal::gesture)>>);

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  lv::pointer mouse{display, lv::signal::gesture};
  check(mouse.raw() != nullptr);
  return failures;
}
