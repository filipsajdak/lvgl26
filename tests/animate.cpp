#include <lv/lv.hpp>

#include "check.hpp"

#include <concepts>
#include <cstdint>
#include <meta>

namespace {

using namespace lv::units;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

void click(lv::obj o) { lv_obj_send_event(o.raw(), LV_EVENT_CLICKED, nullptr); }

// Sets a flag when it is destroyed: shows that a cancelled task cleans up.
struct at_exit {
  bool* flag;
  ~at_exit() { *flag = true; }
};

// What a task has seen of one animation.
struct outcome {
  int steps = 0;         // 1 once the wait is over
  bool finished = false; // what co_await gave
  bool ended = false;    // the task's frame is gone
};

std::int32_t value_of(lv::view<"arc"> arc) { return arc.get<"value">(); }

// The ends and the time arrive at run time: nothing here can be folded.
lv::task<> sweep(lv::view<"arc"> arc, int from, int to, lv::duration time, outcome* out) {
  at_exit cleanup{&out->ended};
  out->finished = co_await lv::animate(arc, {.value = {from, to}}, time);
  out->steps = 1;
}

void an_animation_runs_to_its_end(lv::obj screen) {
  auto arc = screen.add<"arc">();
  const auto before = free_bytes();
  outcome out;
  check(lv::spawn(sweep(arc, 10, 90, 300 * ms, &out)));
  check(value_of(arc) == 10);  // the start value is set at once
  check(out.steps == 0 && lv_anim_count_running() == 1);

  lv_test_wait(150);
  check(value_of(arc) > 10 && value_of(arc) < 90);
  check(out.steps == 0);

  lv_test_wait(200);
  check(value_of(arc) == 90);
  check(out.steps == 1 && out.finished && out.ended);
  check(lv_anim_count_running() == 0);
  check(free_bytes() == before);  // no animation, timer or frame is left

  // It can run downwards, and with no time at all.
  outcome down;
  check(lv::spawn(sweep(arc, 80, 20, 100 * ms, &down)));
  lv_test_wait(50);
  check(value_of(arc) < 80 && value_of(arc) > 20);
  lv_test_wait(100);
  check(value_of(arc) == 20 && down.finished);

  outcome instant;
  check(lv::spawn(sweep(arc, 0, 55, 0 * ms, &instant)));
  check(value_of(arc) == 55 && instant.finished && instant.ended);  // at once

  // With no time, it still takes the property over from an animation that runs.
  outcome slow, jump;
  check(lv::spawn(sweep(arc, 0, 100, 200 * ms, &slow)));
  lv_test_wait(50);
  check(lv::spawn(sweep(arc, 0, 70, 0 * ms, &jump)));
  check(jump.finished && lv_anim_count_running() == 0);
  lv_test_wait(1);
  check(slow.steps == 1 && !slow.finished);
  lv_test_wait(200);
  check(value_of(arc) == 70);
  check(free_bytes() == before);
  lv_obj_delete(arc.raw());
}

// Lengths, in pixels and in percent of the parent, and an angle.
lv::task<> resize(lv::obj box, int from, int to, bool* done) {
  co_await lv::animate(box, {.width = {from * px, to * px}}, 100 * ms);
  co_await lv::animate(box, {.height = {from * pct, to * pct}}, 100 * ms);
  co_await lv::animate(box, {.y = {-from * pct, -to * pct}}, 100 * ms);
  *done = true;
}

lv::task<> turn(lv::view<"arc"> arc, int from, int to, bool* done) {
  co_await lv::animate(arc, {.rotation = {from * deg, to * deg}}, 100 * ms);
  *done = true;
}

// An easing curve as LVGL counts it, and which curves it can follow.
static_assert(lv::detail::bezier_part(0) == 0 && lv::detail::bezier_part(1) == 1024);
static_assert(lv::detail::bezier_part(0.5) == 512 && lv::detail::bezier_part(-0.25) == -256);
static_assert(lv::detail::bezier_part(0.33) == 338 && lv::detail::bezier_part(-0.33) == -338);
static_assert(lv::detail::bezier_part(0.7) == 717 && lv::detail::bezier_part(-0.7) == -717);  // rounded, not cut
// A y is whatever LVGL's 16 bits hold: -32 to just under 32.
static_assert(lv::detail::fits_bezier(0) && lv::detail::fits_bezier(-32) && lv::detail::fits_bezier(31.999));
static_assert(!lv::detail::fits_bezier(-32.001) && !lv::detail::fits_bezier(32));
static_assert(!lv::detail::fits_bezier(31.9999));  // rounds to 32768, one more than 16 bits hold
static_assert(lv::detail::usable(lv::bezier{}) && lv::detail::usable(lv::bezier{0, 0, 1, 1}));
static_assert(lv::detail::usable(lv::bezier{0.3, -32, 0.7, 31.999}));
static_assert(lv::detail::usable(lv::bezier{0, 0, 0, 1}) && lv::detail::usable(lv::bezier{1, 0, 1, 1}));  // x at its ends
static_assert(lv::detail::usable(lv::bezier{0.3, 31.999, 0.7, -32}));  // either y may overshoot either way
static_assert(!lv::detail::usable(lv::bezier{-0.1, 0, 1, 1}) && !lv::detail::usable(lv::bezier{1.1, 0, 1, 1}));
static_assert(!lv::detail::usable(lv::bezier{0, 0, -0.1, 1}) && !lv::detail::usable(lv::bezier{0, 0, 1.1, 1}));
static_assert(!lv::detail::usable(lv::bezier{0, -33, 1, 1}) && !lv::detail::usable(lv::bezier{0, 33, 1, 1}));
static_assert(!lv::detail::usable(lv::bezier{0, 0, 1, -33}) && !lv::detail::usable(lv::bezier{0, 0, 1, 33}));

// What an easing is: one speed, a curve, or one of LVGL's paths.
static_assert(!lv::easing{}.is_curve() && !lv::ease::linear.is_curve());
static_assert(!lv::easing{lv::bezier{}}.is_curve());  // a straight line is one speed
constexpr lv::easing fall{lv::bezier{0.33, 0, 0.67, 0.33}};
static_assert(fall.is_curve() && fall.curve() == lv::bezier{0.33, 0, 0.67, 0.33});
static_assert(!lv::ease::in.is_curve() && !lv::ease::in_out.is_curve() && !lv::ease::bounce.is_curve());

// The path LVGL is given. Checked at run time: with -fsanitize=undefined GCC does not
// compare the addresses of two functions in a constant expression.
void easings_name_lvgl_paths() {
  check(lv::easing{}.path() == &lv_anim_path_linear && lv::ease::linear.path() == &lv_anim_path_linear);
  check(lv::easing{lv::bezier{}}.path() == &lv_anim_path_linear);
  check(fall.path() == &lv_anim_path_custom_bezier3);
  check(lv::ease::in_quad.path() == &lv_anim_path_custom_bezier3);
  check(lv::ease::in.path() == &lv_anim_path_ease_in && lv::ease::out.path() == &lv_anim_path_ease_out);
  check(lv::ease::in_out.path() == &lv_anim_path_ease_in_out);
  check(lv::ease::overshoot.path() == &lv_anim_path_overshoot);
  check(lv::ease::bounce.path() == &lv_anim_path_bounce && lv::ease::step.path() == &lv_anim_path_step);
}

// Every animation path LVGL has is named in lv::ease. A new one in LVGL fails here.
consteval std::size_t lvgl_paths() {
  std::size_t n = 0;
  for (auto f : lv::detail::api)
    if (std::meta::identifier_of(f).starts_with("lv_anim_path_")) ++n;
  return n;
}
// linear, in, out, in_out, overshoot, bounce and step, and the one behind every curve.
static_assert(lvgl_paths() == 8);

// A named curve has the numbers LVGL gives it.
constexpr bool has_parts(const lv::easing& e, int x1, int y1, int x2, int y2) {
  const auto& c = e.curve();
  return e.is_curve() && lv::detail::bezier_part(c.x1) == x1 && lv::detail::bezier_part(c.y1) == y1 &&
         lv::detail::bezier_part(c.x2) == x2 && lv::detail::bezier_part(c.y2) == y2;
}
static_assert(has_parts(lv::ease::in_sine, LV_BEZIER_VAL_FLOAT(0.12), 0, LV_BEZIER_VAL_FLOAT(0.39), 0));
static_assert(has_parts(lv::ease::out_quad, LV_BEZIER_VAL_FLOAT(0.5), 1024, LV_BEZIER_VAL_FLOAT(0.89), 1024));
static_assert(has_parts(lv::ease::in_out_cubic, LV_BEZIER_VAL_FLOAT(0.65), 0, LV_BEZIER_VAL_FLOAT(0.35), 1024));
static_assert(has_parts(lv::ease::in_circ, LV_BEZIER_VAL_FLOAT(0.55), 0, 1024, LV_BEZIER_VAL_FLOAT(0.45)));
static_assert(has_parts(lv::ease::in_back, LV_BEZIER_VAL_FLOAT(0.36), 0, LV_BEZIER_VAL_FLOAT(0.66),
                        LV_BEZIER_VAL_FLOAT(-0.56)));
static_assert(has_parts(lv::ease::out_back, LV_BEZIER_VAL_FLOAT(0.34), LV_BEZIER_VAL_FLOAT(1.56),
                        LV_BEZIER_VAL_FLOAT(0.64), 1024));
// Each of the 24 named curves is a curve that LVGL can follow.
constexpr const lv::easing* named[] = {
    &lv::ease::in_sine,  &lv::ease::out_sine,  &lv::ease::in_out_sine,  &lv::ease::in_quad,
    &lv::ease::out_quad, &lv::ease::in_out_quad, &lv::ease::in_cubic,   &lv::ease::out_cubic,
    &lv::ease::in_out_cubic, &lv::ease::in_quart, &lv::ease::out_quart, &lv::ease::in_out_quart,
    &lv::ease::in_quint, &lv::ease::out_quint, &lv::ease::in_out_quint, &lv::ease::in_expo,
    &lv::ease::out_expo, &lv::ease::in_out_expo, &lv::ease::in_circ,    &lv::ease::out_circ,
    &lv::ease::in_out_circ, &lv::ease::in_back, &lv::ease::out_back,    &lv::ease::in_out_back};
constexpr bool all_usable() {
  for (const lv::easing* e : named)
    if (!e->is_curve() || !lv::detail::usable(e->curve())) return false;
  return true;
}
static_assert(all_usable());

lv::task<> eased(lv::view<"arc"> arc, lv::easing how, outcome* out) {
  out->finished = co_await lv::animate(arc, {.value = {0, 1000}}, 400 * ms, how);
  out->steps = 1;
}

// The value half way through an animation from 0 to 1000 with this easing. The
// easing changes how the value moves, never where it ends.
std::int32_t half_way(lv::view<"arc"> arc, lv::easing how) {
  outcome out;
  check(lv::spawn(eased(arc, how, &out)));
  lv_test_wait(200);
  const std::int32_t middle = value_of(arc);
  check(out.steps == 0);
  lv_test_wait(250);
  check(value_of(arc) == 1000 && out.steps == 1 && out.finished);
  return middle;
}

void an_easing_shapes_the_motion(lv::obj screen) {
  auto arc = screen.add<"arc">();
  arc.set({.range = {0, 1000}});
  const auto before = free_bytes();

  const auto straight = half_way(arc, {});
  check(straight > 440 && straight < 560);
  check(half_way(arc, lv::ease::linear) == straight);

  // A curve. Like a fall under gravity: slow first. This one is the parabola t * t.
  const auto falling = half_way(arc, lv::bezier{0.33, 0, 0.67, 0.33});
  check(falling > 180 && falling < 330);
  // Like a rise: fast first.
  const auto rising = half_way(arc, lv::bezier{0.33, 0.67, 0.67, 1});
  check(rising > 670 && rising < 820);
  // A design token of an easing curve is given as it is.
  const lv::token<lv::bezier> from_tokens{lv::bezier{0.33, 0, 0.67, 0.33}, lv::detail::from_design_tokens{}};
  check(half_way(arc, from_tokens) == falling);

  // LVGL's paths and named curves.
  check(half_way(arc, lv::ease::in) < 400);
  check(half_way(arc, lv::ease::out) > 600);
  const auto both = half_way(arc, lv::ease::in_out);
  check(both > 400 && both < 600);
  check(half_way(arc, lv::ease::in_quad) < 400);
  check(half_way(arc, lv::ease::out_quad) > 600);
  check(half_way(arc, lv::ease::in_expo) < 150);
  check(half_way(arc, lv::ease::overshoot) > 500);
  check(half_way(arc, lv::ease::step) == 0);  // nothing until the end
  const auto bouncing = half_way(arc, lv::ease::bounce);
  check(bouncing > 500 && bouncing <= 1000);

  check(lv_anim_count_running() == 0 && free_bytes() == before);
  lv_obj_delete(arc.raw());
}

void every_kind_of_number_is_animated(lv::obj screen) {
  auto parent = screen.add<"obj">();
  parent.set({.height = 200 * px, .width = 300 * px});
  auto box = parent.add<"obj">();
  bool done = false;
  check(lv::spawn(resize(box, 20, 60, &done)));
  lv_test_wait(50);
  check(lv_obj_get_width(box.raw()) > 20 && lv_obj_get_width(box.raw()) < 60);
  lv_test_wait(100);
  check(lv_obj_get_width(box.raw()) == 60);  // the first is over, the second runs
  lv_test_wait(110);
  const auto content_height = lv_obj_get_content_height(parent.raw());
  check(lv_obj_get_height(box.raw()) == content_height * 60 / 100);
  check(!done);
  lv_test_wait(110);
  check(done);
  check(lv_obj_get_y_aligned(box.raw()) == LV_PCT(-60));

  auto arc = screen.add<"arc">();
  bool turned = false;
  check(lv::spawn(turn(arc, 0, 270, &turned)));
  lv_test_wait(50);
  check(lv_arc_get_rotation(arc.raw()) > 0 && lv_arc_get_rotation(arc.raw()) < 270);
  lv_test_wait(100);
  check(turned && lv_arc_get_rotation(arc.raw()) == 270);

  lv_obj_delete(parent.raw());
  lv_obj_delete(arc.raw());
}

// The checks behind the two ends, with values the compiler cannot fold.
void ends_are_checked(int a, int b) {
  using lv::detail::can_run_between;
  using lv::detail::length_scale;
  using lv::detail::scale_of;
  check(scale_of(a * px) == length_scale::pixels);
  check(scale_of(-a * px) == length_scale::pixels);
  check(scale_of(a * pct) == length_scale::percent);
  check(scale_of(-a * pct) == length_scale::negative_percent);
  check(scale_of(lv::size_content) == length_scale::content);

  check(can_run_between(a * px, b * px));
  check(can_run_between(a * px, -b * px));
  check(can_run_between(lv::length{a * pct}, lv::length{b * pct}));
  check(can_run_between(lv::length{-a * pct}, lv::length{-b * pct}));
  check(!can_run_between(a * px, b * pct));
  check(!can_run_between(lv::length{a * pct}, lv::length{-b * pct}));
  check(!can_run_between(lv::size_content, lv::size_content));
  check(can_run_between(lv::horizontal{a * px}, lv::horizontal{b * px}));
  check(!can_run_between(lv::vertical{a * px}, lv::vertical{b * pct}));
  check(can_run_between(a * deg, b * deg));
  check(can_run_between(lv::opa{a * pct}, lv::opa{b * pct}));

  const std::uint32_t large = 0x80000000u + static_cast<std::uint32_t>(a);
  check(can_run_between(lv::strict<std::uint32_t>{static_cast<std::uint32_t>(a)},
                        lv::strict<std::uint32_t>{static_cast<std::uint32_t>(b)}));
  check(!can_run_between(lv::strict<std::uint32_t>{large}, lv::strict<std::uint32_t>{0u}));
  check(!can_run_between(lv::strict<std::uint32_t>{0u}, lv::strict<std::uint32_t>{large}));

  const lv::from_to<lv::angle> unset;
  const lv::from_to<lv::angle> set{a * deg, b * deg};
  check(!unset.is_set() && set.is_set());
  check(set.from() == a * deg && set.to() == b * deg);

  lv::animatable<"arc"> what{};
  check(lv::detail::count_set<"arc">(what) == 0);
  what.value = {a, b};
  check(lv::detail::count_set<"arc">(what) == 1);
  what.rotation = {a * deg, b * deg};
  check(lv::detail::count_set<"arc">(what) == 2);
}

// Whichever comes first decides; the animation that loses is deleted.
lv::task<> sweep_or_timeout(lv::view<"arc"> arc, lv::duration limit, int* answer, bool* ended) {
  at_exit cleanup{ended};
  *answer = co_await lv::first(
      lv::animate(arc, {.value = {0, 100}}, 200 * ms).then([](bool finished) {
        return finished ? 1 : -1;
      }),
      lv::sleep(limit).then([] { return 2; }));
}

// Without handlers, lv::first says which one won.
lv::task<> sweep_or_click(lv::view<"arc"> arc, lv::view<"button"> button, std::size_t* which) {
  *which = co_await lv::first(lv::animate(arc, {.value = {0, 100}}, 200 * ms),
                              button.next().clicked);
}

// A handler that does not look at how the animation ended.
lv::task<> sweep_then_count(lv::view<"arc"> arc, int* count) {
  co_await lv::first(lv::animate(arc, {.value = {0, 100}}, 50 * ms).then([&] { ++*count; }));
}

void first_of_several(lv::obj screen) {
  auto arc = screen.add<"arc">();
  auto button = screen.add<"button">();
  const auto before = free_bytes();

  int answer = 0;
  bool ended = false;
  check(lv::spawn(sweep_or_timeout(arc, 500 * ms, &answer, &ended)));
  lv_test_wait(250);
  check(answer == 1 && ended && value_of(arc) == 100);
  lv_test_wait(400);  // the sleep that lost does not come back
  check(answer == 1);

  answer = 0;
  ended = false;
  check(lv::spawn(sweep_or_timeout(arc, 100 * ms, &answer, &ended)));
  lv_test_wait(120);
  check(answer == 2 && ended);
  check(lv_anim_count_running() == 0);  // the animation that lost is deleted
  const auto stopped_at = value_of(arc);
  check(stopped_at > 0 && stopped_at < 100);
  lv_test_wait(200);
  check(value_of(arc) == stopped_at);  // and the property stays where it was

  std::size_t which = 99;
  check(lv::spawn(sweep_or_click(arc, button, &which)));
  lv_test_wait(100);
  click(button);
  check(which == 1 && lv_anim_count_running() == 0);
  check(lv::spawn(sweep_or_click(arc, button, &which)));
  lv_test_wait(250);
  check(which == 0);

  int count = 0;
  check(lv::spawn(sweep_then_count(arc, &count)));
  lv_test_wait(80);
  check(count == 1);

  check(free_bytes() == before);
  lv_obj_delete(arc.raw());
  lv_obj_delete(button.raw());
}

// Deleting the widget ends the task, while it waits for the animation and later.
lv::task<> sweep_then_sleep(lv::view<"arc"> arc, outcome* out) {
  at_exit cleanup{&out->ended};
  out->finished = co_await lv::animate(arc, {.value = {0, 100}}, 100 * ms);
  out->steps = 1;
  co_await lv::sleep(60000 * ms);
  out->steps = 2;
}

// A task that waits for a click on one widget and then animates another.
lv::task<> click_then_sweep(lv::view<"button"> button, lv::view<"arc"> arc, outcome* out) {
  at_exit cleanup{&out->ended};
  co_await button.next().clicked;
  out->finished = co_await lv::animate(arc, {.value = {0, 100}}, 100 * ms);
  out->steps = 1;
}

// A task that deletes the widget and then asks for an animation of it.
lv::task<> deletes_then_sweeps(lv::view<"arc"> arc, outcome* out) {
  at_exit cleanup{&out->ended};
  co_await arc.next().clicked;
  lv_obj_delete(arc.raw());
  out->steps = 1;
  out->finished = co_await lv::animate(arc, {.value = {0, 100}}, 100 * ms);  // must not touch it
  out->steps = 2;
}

// A task that wants to see the deletion waits for it.
lv::task<> sweep_or_gone(lv::view<"arc"> arc, int* answer) {
  *answer = co_await lv::first(lv::animate(arc, {.value = {0, 100}}, 100 * ms).then([] {
                                 return 1;
                               }),
                               arc.next().gone.then([] { return 2; }));
}

void cancelling_deletes_the_animation(lv::obj screen) {
  const auto before = free_bytes();
  {
    // The widget is deleted in the middle of its animation.
    auto arc = screen.add<"arc">();
    outcome out;
    check(lv::spawn(sweep_then_sleep(arc, &out)));
    lv_test_wait(50);
    check(lv_anim_count_running() == 1 && !out.ended);
    lv_obj_delete(arc.raw());
    check(out.ended && out.steps == 0);
    check(lv_anim_count_running() == 0);
    lv_test_wait(200);
    check(out.steps == 0);  // nothing of the ended task ran later
  }
  {
    // The widget is deleted after its animation: the task is still tied to it.
    auto arc = screen.add<"arc">();
    outcome out;
    check(lv::spawn(sweep_then_sleep(arc, &out)));
    lv_test_wait(150);
    check(out.steps == 1 && out.finished && !out.ended);
    lv_obj_delete(arc.raw());
    check(out.ended && out.steps == 1);
  }
  {
    // Another widget the task has waited on is deleted: the animation goes too.
    auto button = screen.add<"button">();
    auto arc = screen.add<"arc">();
    outcome out;
    check(lv::spawn(click_then_sweep(button, arc, &out)));
    click(button);
    lv_test_wait(50);
    check(lv_anim_count_running() == 1);
    lv_obj_delete(button.raw());
    check(out.ended && out.steps == 0 && lv_anim_count_running() == 0);
    const auto stopped_at = value_of(arc);
    lv_test_wait(100);
    check(value_of(arc) == stopped_at);
    lv_obj_delete(arc.raw());
  }
  {
    auto arc = screen.add<"arc">();
    outcome out;
    check(lv::spawn(deletes_then_sweeps(arc, &out)));
    click(arc);
    check(out.steps == 1 && out.ended);
    check(lv_anim_count_running() == 0);
  }
  {
    auto arc = screen.add<"arc">();
    int answer = 0;
    check(lv::spawn(sweep_or_gone(arc, &answer)));
    lv_test_wait(50);
    lv_obj_delete(arc.raw());
    check(answer == 2 && lv_anim_count_running() == 0);
  }
  lv_test_wait(50);
  check(free_bytes() == before);
}

// Sweeps again and again, and counts how often and how the sweeps ended.
struct tally {
  int finished = 0;
  int stopped = 0;
  bool ended = false;
};
lv::task<> pulse(lv::view<"arc"> arc, int rounds, tally* out) {
  at_exit cleanup{&out->ended};
  for (int i = 0; i < rounds; ++i) {
    const bool finished = co_await lv::animate(arc, {.value = {0, 100}}, 100 * ms);
    ++(finished ? out->finished : out->stopped);
  }
}

void a_stopped_animation_ends_the_wait(lv::obj screen) {
  auto arc = screen.add<"arc">();
  const auto before = free_bytes();
  {
    // lv_anim_delete() from outside: the wait is over in the next turn, with false.
    outcome out;
    check(lv::spawn(sweep(arc, 0, 100, 200 * ms, &out)));
    lv_test_wait(50);
    check(lv_anim_delete(arc.raw(), nullptr));
    check(out.steps == 0);  // not inside lv_anim_delete
    lv_test_wait(1);
    check(out.steps == 1 && !out.finished && out.ended);
  }
  {
    // A newer animation of the same property takes over, as in LVGL.
    outcome older, newer;
    check(lv::spawn(sweep(arc, 0, 100, 200 * ms, &older)));
    lv_test_wait(50);
    check(lv::spawn(sweep(arc, 40, 60, 100 * ms, &newer)));
    check(lv_anim_count_running() == 1 && value_of(arc) == 40);
    check(older.steps == 0);
    lv_test_wait(1);
    check(older.steps == 1 && !older.finished);
    check(newer.steps == 0);
    lv_test_wait(120);
    check(newer.steps == 1 && newer.finished && value_of(arc) == 60);
  }
  {
    // Another property of the same widget is another animation: both run.
    outcome value;
    bool turned = false;
    check(lv::spawn(sweep(arc, 0, 100, 100 * ms, &value)));
    check(lv::spawn(turn(arc, 0, 90, &turned)));
    check(lv_anim_count_running() == 2);
    lv_test_wait(120);
    check(value.finished && turned);
  }
  {
    // Two tasks that keep taking the property from each other both get on.
    tally a, b;
    check(lv::spawn(pulse(arc, 3, &a)));
    lv_test_wait(30);
    check(lv::spawn(pulse(arc, 3, &b)));
    lv_test_wait(1000);
    check(a.ended && b.ended);
    check(a.finished + a.stopped == 3 && b.finished + b.stopped == 3);
    check(a.stopped > 0);
    check(lv_anim_count_running() == 0);
  }
  {
    // A stopped wait that is cancelled before its next turn leaves nothing behind.
    outcome out;
    auto other = screen.add<"arc">();
    check(lv::spawn(sweep(other, 0, 100, 200 * ms, &out)));
    lv_test_wait(50);
    check(lv_anim_delete(other.raw(), nullptr));
    lv_obj_delete(other.raw());
    check(out.ended && out.steps == 0);
    lv_test_wait(10);
    check(out.steps == 0);
  }
  check(free_bytes() == before);
  lv_obj_delete(arc.raw());
}

// The same property twice in one lv::first: the second takes over from the first,
// which is then the first to be over.
lv::task<> same_property_twice(lv::view<"arc"> arc, std::size_t* which) {
  *which = co_await lv::first(lv::animate(arc, {.value = {0, 100}}, 100 * ms),
                              lv::animate(arc, {.value = {50, 60}}, 100 * ms));
}

void the_same_property_twice_in_first(lv::obj screen) {
  auto arc = screen.add<"arc">();
  const auto before = free_bytes();
  std::size_t which = 99;
  check(lv::spawn(same_property_twice(arc, &which)));
  check(which == 99 && value_of(arc) == 50);
  lv_test_wait(1);
  check(which == 0);
  check(lv_anim_count_running() == 0);
  check(free_bytes() == before);
  lv_obj_delete(arc.raw());
}

template <class A>
concept has_range = requires(A a) { a.range; };
template <class A>
concept has_mode = requires(A a) { a.mode; };
template <class A>
concept has_text = requires(A a) { a.text; };
template <class A>
concept has_on_clicked = requires(A a) { a.on_clicked; };
template <class A>
concept has_style = requires(A a) { a.style; };

}  // namespace

int main(int argc, char**) {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  // The struct lists the properties that take one number, and only those.
  static_assert(std::same_as<decltype(lv::animatable<"arc">{}.value),
                             lv::from_to<lv::strict<std::int32_t>>>);
  static_assert(std::same_as<decltype(lv::animatable<"arc">{}.rotation), lv::from_to<lv::angle>>);
  static_assert(std::same_as<decltype(lv::animatable<"arc">{}.width), lv::from_to<lv::horizontal>>);
  static_assert(std::same_as<decltype(lv::animatable<"label">{}.y), lv::from_to<lv::vertical>>);
  static_assert(!has_range<lv::animatable<"arc">>);       // two values
  static_assert(!has_mode<lv::animatable<"arc">>);        // an enum
  static_assert(!has_text<lv::animatable<"label">>);      // text
  static_assert(!has_on_clicked<lv::animatable<"obj">>);  // an event
  static_assert(!has_style<lv::animatable<"obj">>);

  an_animation_runs_to_its_end(screen);
  easings_name_lvgl_paths();
  an_easing_shapes_the_motion(screen);
  every_kind_of_number_is_animated(screen);
  ends_are_checked(argc + 4, argc + 9);
  first_of_several(screen);
  cancelling_deletes_the_animation(screen);
  a_stopped_animation_ends_the_wait(screen);
  the_same_property_twice_in_first(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
