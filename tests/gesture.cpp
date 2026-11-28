// lv/gesture.hpp: a swipe gives its direction as a handler and as a wait, a hold gives its progress, and both
// follow LVGL's rules for scrolling. The pointer is fed sample by sample, one read of LVGL for each.
#include <lv/gesture.hpp>
#include <lv/lv.hpp>
#include <lv/pointer.hpp>

#include "check.hpp"

#include <cstdio>
#include <type_traits>
#include <utility>

namespace {

using namespace lv::units;

int swipes = 0;
lv::swipe last_swipe = lv::swipe::left;
void on_swipe(lv::swipe direction) {
  ++swipes;
  last_swipe = direction;
}

// A page with a button, a list that scrolls up and down, and room below.
//! [page]
constexpr auto ui = lv::column(
    {.id = "page", .scroll_dir = "none", .size = {320 * px, 240 * px}, .on_swiped = &on_swipe},
    lv::button({.id = "ring", .size = {100 * px, 40 * px}}),
    lv::box({.id = "scroller", .scroll_dir = "ver", .size = {300 * px, 100 * px}},
            lv::box({.id = "tall", .size = {300 * px, 400 * px}})),
    lv::box({.id = "quiet", .align = "bottom_right", .gesture_bubble = false, .size = {80 * px, 20 * px}}));
//! [page]

constinit lv::frame<320, 240> frame;

// A wait for a swipe: what it saw, and how many times it ended.
//! [waits]
int waited = 0;
lv::swipe waited_for = lv::swipe::left;
lv::task<> wait_for_a_swipe(lv::obj page) {
  waited_for = co_await lv::swiped(page);
  ++waited;
}
lv::task<> wait_for_left_or_right(lv::obj page) {
  waited_for = co_await lv::swiped<lv::swipe::left, lv::swipe::right>(page);
  ++waited;
}
//! [waits]

// A hold, and what it gave.
//! [hold]
lv::state<lv::percent> progress{lv::percent{0}};
int hold_result = -1;  // -1: not done, 0: false, 1: true
lv::task<> hold_the_ring(lv::obj ring, lv::duration time) {
  hold_result = co_await lv::hold(ring, time, progress) ? 1 : 0;
}
//! [hold]

lv::point middle(lv::obj widget) {
  lv_area_t at;
  lv_obj_get_coords(widget.raw(), &at);
  return {(at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2};
}

// One read of LVGL, `40 ms` after the one before.
void read(lv::application& app) {
  app.advance(40 * ms);
  app.step();
}

// A finger down at `from`, then `steps` moves of `dx` and `dy` each, one read each, and up again.
void drag(lv::application& app, lv::pointer<>& mouse, lv::point from, std::int32_t dx, std::int32_t dy, int steps) {
  mouse.press(from);
  read(app);
  for (int i = 1; i <= steps; ++i) {
    mouse.move({from.x + dx * i, from.y + dy * i});
    read(app);
  }
  mouse.release();
  read(app);
}

// The thresholds are the pointer's: a swipe of 30 pixels at 2 pixels a read for a pointer that is told so, and no
// swipe of that kind for one with LVGL's defaults. A struct of other members is no gesture settings.
struct own_limits {
  lv::length swipe_distance = lv::length::px(30);
  lv::length swipe_speed = lv::length::px(2);
  lv::length scroll_limit = lv::length::px(10);
  lv::duration long_press = lv::duration{400};
};
static_assert(lv::gesture_settings<own_limits> && lv::gesture_settings<lv::gesture_limits>);
static_assert(!lv::gesture_settings<int> && !lv::gesture_settings<lv::point>);
static_assert(std::is_same_v<decltype(lv::pointer{std::declval<lv::display&>(), own_limits{}}), lv::pointer<16>>);

void reset() {
  swipes = 0;
  waited = 0;
  hold_result = -1;
  progress.set(lv::percent{0});
}

// A swipe goes to the nearest widget that waits for it, so a wait above it sees nothing of what the page
// takes; and a widget that keeps gestures with no wait turns the swipes off for what lies in it.
void the_nearest_wait_gets_the_swipe(lv::application& app, lv::pointer<>& mouse, lv::obj quiet) {
  reset();
  auto holder = lv::screen_active().add<"obj">();
  check(lv::spawn(wait_for_a_swipe(lv::screen_active()), holder));
  drag(app, mouse, {160, 215}, 10, 0, 6);
  check(swipes == 1 && waited == 0);  // the page took it; the screen's wait still waits
  drag(app, mouse, middle(quiet), -10, 0, 6);
  check(swipes == 1 && waited == 0);  // turned off for the quiet widget: nobody sees it
  lv_obj_delete(holder.raw());
  lv_obj_set_gesture_bubble(lv::screen_active().raw(), true);
}

// Sixty pixels in six reads, in each direction, give one swipe of that direction.
void a_swipe_has_its_direction(lv::application& app, lv::pointer<>& mouse, lv::obj page) {
  const lv::point blank{160, 215};
  reset();
  drag(app, mouse, blank, 10, 0, 6);
  check(swipes == 1 && last_swipe == lv::swipe::right);
  drag(app, mouse, blank, -10, 0, 6);
  check(swipes == 2 && last_swipe == lv::swipe::left);
  drag(app, mouse, {160, 170}, 0, 10, 6);
  check(swipes == 3 && last_swipe == lv::swipe::down);
  drag(app, mouse, {160, 225}, 0, -10, 6);
  check(swipes == 4 && last_swipe == lv::swipe::up);
  (void)page;
}

// A short drag, and a drag that is slow, are no swipe.
void the_limits_are_the_pointers(lv::application& app, lv::display& display) {
  reset();
  const lv::point blank{160, 215};
  {
    lv::pointer easy{display, own_limits{}};
    drag(app, easy, blank, 10, 0, 4);  // 40 px: a swipe for this pointer
    check(swipes == 1 && last_swipe == lv::swipe::right);
    drag(app, easy, blank, 2, 0, 20);  // 40 px at 2 px a read
    check(swipes == 2);
  }
  {
    lv::pointer strict{display, lv::gesture_limits{.swipe_distance = lv::length::px(100)}};
    drag(app, strict, blank, 10, 0, 8);  // 80 px: not enough for this one
    check(swipes == 2);
  }
  {
    lv::pointer plain{display};
    drag(app, plain, blank, 10, 0, 4);  // 40 px: LVGL's 50 px is not reached
    check(swipes == 2);
  }
}

void a_short_or_slow_drag_is_no_swipe(lv::application& app, lv::pointer<>& mouse) {
  const lv::point blank{160, 215};
  reset();
  drag(app, mouse, blank, 10, 0, 4);  // 40 px: under the distance
  check(swipes == 0);
  drag(app, mouse, blank, 2, 0, 40);  // 80 px, but 2 px a read: under the speed
  check(swipes == 0);
}

// A task that waits for a swipe gets its direction; a wait for two directions leaves the others alone.
void a_task_waits_for_a_swipe(lv::application& app, lv::pointer<>& mouse, lv::obj page) {
  const lv::point blank{160, 215};
  reset();
  check(lv::spawn(wait_for_a_swipe(page), page));
  drag(app, mouse, blank, 0, 10, 6);
  check(waited == 1 && waited_for == lv::swipe::down);

  reset();
  check(lv::spawn(wait_for_left_or_right(page), page));
  drag(app, mouse, {160, 225}, 0, -10, 6);  // up: not wanted
  check(waited == 0);
  drag(app, mouse, blank, -10, 0, 6);
  check(waited == 1 && waited_for == lv::swipe::left);
}

// A list that scrolls up and down takes a vertical drag as a scroll, and gives no swipe for it. Across the
// list a swipe goes sideways.
void a_scroll_is_no_swipe(lv::application& app, lv::pointer<>& mouse, lv::obj scroller) {
  reset();
  const lv::point inside = middle(scroller);
  const auto before = lv_obj_get_scroll_y(scroller.raw());
  drag(app, mouse, inside, 0, -10, 6);
  check(lv_obj_get_scroll_y(scroller.raw()) > before);
  check(swipes == 0);
  drag(app, mouse, inside, 10, 0, 6);
  check(swipes == 1 && last_swipe == lv::swipe::right);
  lv_obj_scroll_to_y(scroller.raw(), 0, LV_ANIM_OFF);
}

// A hold that lasts its time gives true, with the progress at the full value.
void a_hold_that_lasts(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  check(lv::spawn(hold_the_ring(ring, 1000 * ms), page));
  mouse.press(middle(ring));
  read(app);
  check(hold_result == -1);
  for (int i = 0; i < 12; ++i) read(app);  // 480 ms
  check(hold_result == -1 && progress.get().value > 30 && progress.get().value < 60);
  for (int i = 0; i < 20; ++i) read(app);  // past the second
  check(hold_result == 1 && progress.get().value == 100);
  mouse.release();
  read(app);
}

// A finger lifted before the time gives false, and the progress goes back to nothing.
void a_hold_that_is_cut_short(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  check(lv::spawn(hold_the_ring(ring, 1000 * ms), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 10; ++i) read(app);
  check(hold_result == -1 && progress.get().value > 0);
  mouse.release();
  read(app);
  check(hold_result == 0 && progress.get().value == 0);
  for (int i = 0; i < 40; ++i) read(app);
  check(hold_result == 0 && progress.get().value == 0);  // the animation is gone
}

// A drag that becomes a scroll ends the hold with false.
void a_hold_that_becomes_a_scroll(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj tall) {
  reset();
  check(lv::spawn(hold_the_ring(tall, 1000 * ms), page));
  drag(app, mouse, middle(lv::obj{lv_obj_get_parent(tall.raw())}), 0, -10, 6);
  for (int i = 0; i < 40; ++i) read(app);
  check(hold_result == 0 && progress.get().value == 0);
  lv_obj_scroll_to_y(lv_obj_get_parent(tall.raw()), 0, LV_ANIM_OFF);
}

// A task that ends with its owner leaves nothing: no handler on the widget, no animation.
void nothing_is_left(lv::application& app) {
  const auto round = [&] {
    auto owner = lv::screen_active().add<"obj">();
    auto button = owner.add<"button">();
    check(lv::spawn(hold_the_ring(button, 5000 * ms), owner));
    check(lv::spawn(wait_for_a_swipe(owner), owner));
    lv_obj_delete(owner.raw());
    read(app);
  };
  round();  // the first rounds may allocate or free what LVGL keeps
  round();
  const auto before = app.heap().used;
  round();
  check(app.heap().used == before);
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  lv::pointer mouse{display};
  read(app);  // the layout: the widgets get their places

  auto page = root->get<"page">();
  a_swipe_has_its_direction(app, mouse, page);
  the_nearest_wait_gets_the_swipe(app, mouse, root->get<"quiet">());
  a_short_or_slow_drag_is_no_swipe(app, mouse);
  the_limits_are_the_pointers(app, display);
  a_task_waits_for_a_swipe(app, mouse, page);
  a_scroll_is_no_swipe(app, mouse, root->get<"scroller">());
  a_hold_that_lasts(app, mouse, page, root->get<"ring">());
  a_hold_that_is_cut_short(app, mouse, page, root->get<"ring">());
  a_hold_that_becomes_a_scroll(app, mouse, page, root->get<"tall">());
  nothing_is_left(app);

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
