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

// A wait for one direction only, a hold with no progress to show, and both as items of lv::first, whose
// handlers take what the wait gives or nothing.
lv::task<> wait_for_down(lv::obj page) {
  waited_for = co_await lv::swiped<lv::swipe::down>(page);
  ++waited;
}
lv::task<> hold_without_progress(lv::obj ring, lv::duration time) {
  hold_result = co_await lv::hold(ring, time) ? 1 : 0;
}
int first_result = 0;
lv::task<> swipe_or_hold_with_values(lv::obj page, lv::obj ring) {
  first_result = co_await lv::first(lv::swiped(page).then([](lv::swipe s) { return s == lv::swipe::right ? 1 : 2; }),
                                    lv::hold(ring, 200 * ms).then([](bool held) { return held ? 3 : 4; }));
}
lv::task<> swipe_or_hold_plain(lv::obj page, lv::obj ring) {
  first_result = co_await lv::first(lv::swiped(page).then([] { return 5; }), lv::hold(ring, 200 * ms).then([] { return 6; }));
}

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


// A wait for one direction leaves the others alone, also the ones before it in the enum.
void a_wait_for_one_direction(lv::application& app, lv::pointer<>& mouse, lv::obj page) {
  reset();
  check(lv::spawn(wait_for_down(page), page));
  drag(app, mouse, {160, 215}, -10, 0, 6);  // left
  drag(app, mouse, {160, 215}, 10, 0, 6);   // right
  drag(app, mouse, {160, 225}, 0, -10, 6);  // up
  check(waited == 0);
  drag(app, mouse, {160, 170}, 0, 10, 6);
  check(waited == 1 && waited_for == lv::swipe::down);
}

// A wait on a widget inside the page gets the swipe: the widget keeps it from the page while it waits.
void a_wait_inside_the_page_gets_the_swipe(lv::application& app, lv::pointer<>& mouse, lv::obj tall) {
  reset();
  auto holder = lv::screen_active().add<"obj">();
  check(lv::spawn(wait_for_a_swipe(tall), holder));
  drag(app, mouse, middle(lv::obj{lv_obj_get_parent(tall.raw())}), 10, 0, 6);
  check(waited == 1 && waited_for == lv::swipe::right && swipes == 0);
  lv_obj_delete(holder.raw());
}

// A finger that is down when the hold starts: the time runs from then, and the progress starts at nothing
// whatever it held before. The hold is long, so one read moves it by less than one percent.
void a_hold_that_starts_under_the_finger(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  progress.set(lv::percent{50});
  mouse.press(middle(ring));
  read(app);
  check(lv::spawn(hold_the_ring(ring, 100000 * ms), page));
  check(progress.get().value == 0);
  read(app);
  check(hold_result == -1 && progress.get().value == 0);
  mouse.release();
  read(app);
  check(hold_result == 0);
}

// The form with no progress gives its answer as the other does.
void a_hold_without_progress(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  check(lv::spawn(hold_without_progress(ring, 200 * ms), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 8; ++i) read(app);
  check(hold_result == 1 && progress.get().value == 0);
  mouse.release();
  read(app);
}

// LVGL's release and loss of the press end the hold at once, before the next step of its animation; so does
// the pressed state going without either.
void the_end_of_a_press_ends_the_hold(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  for (const lv_event_code_t code : {LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST}) {
    reset();
    check(lv::spawn(hold_the_ring(ring, 1000 * ms), page));
    mouse.press(middle(ring));
    for (int i = 0; i < 4; ++i) read(app);
    check(hold_result == -1 && progress.get().value > 0);
    lv_obj_send_event(ring.raw(), code, nullptr);
    check(hold_result == 0 && progress.get().value == 0);
    mouse.release();
    read(app);
  }
  reset();
  check(lv::spawn(hold_the_ring(ring, 1000 * ms), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 4; ++i) read(app);
  lv_obj_remove_state(ring.raw(), LV_STATE_PRESSED);
  read(app);
  check(hold_result == 0 && progress.get().value == 0);
  mouse.release();
  read(app);
}

// A second press while the finger is down does not start the time again.
void a_second_press_keeps_the_time(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  check(lv::spawn(hold_the_ring(ring, 1000 * ms), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 12; ++i) read(app);
  lv_obj_send_event(ring.raw(), LV_EVENT_PRESSED, nullptr);
  for (int i = 0; i < 16; ++i) read(app);  // past the second from the first press, not from the second
  check(hold_result == 1);
  mouse.release();
  read(app);
}

// In lv::first, a swipe and a hold give what their handlers return.
void waits_in_first(lv::application& app, lv::pointer<>& mouse, lv::obj page, lv::obj ring) {
  reset();
  check(lv::spawn(swipe_or_hold_with_values(page, ring), page));
  drag(app, mouse, {160, 215}, 10, 0, 6);
  check(first_result == 1);
  check(lv::spawn(swipe_or_hold_with_values(page, ring), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 8; ++i) read(app);
  check(first_result == 3);
  mouse.release();
  read(app);
  check(lv::spawn(swipe_or_hold_plain(page, ring), page));
  drag(app, mouse, {160, 215}, -10, 0, 6);
  check(first_result == 5);
  check(lv::spawn(swipe_or_hold_plain(page, ring), page));
  mouse.press(middle(ring));
  for (int i = 0; i < 8; ++i) read(app);
  check(first_result == 6);
  mouse.release();
  read(app);
}

// A wait whose task ends with its owner leaves no handler and no animation on a widget that stays.
void a_wait_that_ends_leaves_its_widget_alone(lv::application& app, lv::pointer<>& mouse, lv::obj ring) {
  reset();
  const auto handlers = lv_obj_get_event_count(ring.raw());
  auto owner = lv::screen_active().add<"obj">();
  mouse.press(middle(ring));
  read(app);
  check(lv::spawn(hold_the_ring(ring, 1000 * ms), owner));
  check(lv::spawn(wait_for_a_swipe(ring), owner));
  read(app);
  check(progress.get().value > 0 && lv_obj_get_event_count(ring.raw()) > handlers);
  lv_obj_delete(owner.raw());
  const auto stopped = progress.get().value;
  for (int i = 0; i < 4; ++i) read(app);
  check(progress.get().value == stopped && lv_obj_get_event_count(ring.raw()) == handlers);
  mouse.release();
  read(app);
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
  a_wait_for_one_direction(app, mouse, page);
  a_wait_inside_the_page_gets_the_swipe(app, mouse, root->get<"tall">());
  a_hold_that_starts_under_the_finger(app, mouse, page, root->get<"ring">());
  a_hold_without_progress(app, mouse, page, root->get<"ring">());
  the_end_of_a_press_ends_the_hold(app, mouse, page, root->get<"ring">());
  a_second_press_keeps_the_time(app, mouse, page, root->get<"ring">());
  waits_in_first(app, mouse, page, root->get<"ring">());
  a_wait_that_ends_leaves_its_widget_alone(app, mouse, root->get<"ring">());

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
