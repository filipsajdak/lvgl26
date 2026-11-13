#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>
#include <functional>

namespace {

using namespace lv::units;

// A unit is one line. Its values are a number times the unit.
inline constexpr lv::unit<"km/h"> kmh;
inline constexpr lv::unit<"rpm", std::uint16_t> rpm;
using speed_t = lv::quantity<"km/h">;
static_assert(std::is_same_v<decltype(120 * kmh), speed_t>);
static_assert((100 * kmh + 20 * kmh) == 120 * kmh && 50 * kmh < 60 * kmh);
static_assert(sizeof(3000 * rpm) == sizeof(std::uint16_t));
static_assert(!std::is_invocable_v<std::plus<>, speed_t, decltype(1 * rpm)>);  // units do not mix

// Any struct with one integer, bool or enum member also works.
struct trip_id {
  std::uint16_t value;
};
static_assert(lv::detail::strong_value<speed_t> && lv::detail::strong_value<lv::angle>);
static_assert(!lv::detail::strong_value<int> && !lv::detail::strong_value<lv_color_t>);

// Constant-initialized globals: no subject exists until something binds to them.
constinit lv::state speed{40 * kmh};
static_assert(std::is_same_v<decltype(speed), lv::state<speed_t>>);
constinit lv::state<bool> hidden{false};
constinit lv::state<const char*> title{"Trip"};
constinit lv::state<lv_flex_flow_t> flow{LV_FLEX_FLOW_COLUMN};

constexpr auto ui = lv::column({.flex_flow = lv::bind(flow), .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = lv::bind(title)}),
    lv::label({.id = "reading", .text = lv::bind(speed)}),  // shown with its unit
    lv::label({.id = "custom", .text = lv::bind(speed, "now {}")}),
    lv::arc({.id = "dial", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::bar({.id = "bar", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::label({.id = "extra", .text = "x", .user_data = nullptr})];

int c_side_value = -1;
void c_observer(lv_observer_t*, lv_subject_t* s) { c_side_value = lv_subject_get_int(s); }

void before_binding() {
  // A state works as a plain value before any subject exists.
  lv::state<trip_id> local{trip_id{1}};
  local = trip_id{2};
  check(local.get().value == 2);
}

void widgets_follow(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  if (!root) return;

  auto dial = root->get<"dial">();
  auto reading = root->get<"reading">();
  check(dial.get<"value">() == 40);
  check(root->get<"bar">().get<"value">() == 40);
  check(std::strcmp(reading.get<"text">(), "40 km/h") == 0);
  check(std::strcmp(root->get<"custom">().get<"text">(), "now 40") == 0);
  check(std::strcmp(root->get<"title">().get<"text">(), "Trip") == 0);

  speed = 120 * kmh;
  check(dial.get<"value">() == 120);
  check(root->get<"bar">().get<"value">() == 120);
  check(std::strcmp(reading.get<"text">(), "120 km/h") == 0);

  title = "Home";
  check(std::strcmp(root->get<"title">().get<"text">(), "Home") == 0);

  // A property with no LVGL bind function follows through a generic observer.
  check(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  flow = LV_FLEX_FLOW_ROW;
  check(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
  flow = LV_FLEX_FLOW_COLUMN;

  // The arc writes back: a user turning it changes the state.
  dial.set<"value">(75);
  lv_obj_send_event(dial.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(speed.get().value == 75);
  check(root->get<"bar">().get<"value">() == 75);
}

void c_code_shares_the_subject() {
  lv_observer_t* o = lv_subject_add_observer(speed.raw(), &c_observer, nullptr);
  speed = 33 * kmh;
  check(c_side_value == 33);
  lv_subject_set_int(speed.raw(), 44);  // C writes, C++ reads
  check(speed.get().value == 44);
  lv_observer_delete(o);
}

void observers() {
  static int seen = -1;
  lv_observer_t* a = speed.observe([](speed_t v) { seen = v.value; });
  check(seen == speed.get().value);  // called once right away
  speed = 9 * kmh;
  check(seen == 9);

  int captured = -1;
  lv_observer_t* b = speed.observe([&captured](speed_t v) { captured = v.value; });
  speed = 10 * kmh;
  check(captured == 10);
  lv_observer_delete(a);
  lv_observer_delete(b);
  speed = 11 * kmh;
  check(seen == 10 && captured == 10);
}

void runtime_binding(lv::obj screen) {
  auto l = screen.add<"label">().set({.text = lv::bind(speed, "{}")});
  speed = 5 * kmh;
  check(std::strcmp(l.get<"text">(), "5") == 0);

  // Deleting the widget removes its observer: setting the state must be safe.
  lv_obj_delete(l.raw());
  speed = 6 * kmh;
  check(speed.get().value == 6);
}

void survives_a_deleted_tree(lv::obj screen) {
  { auto root = lv::mount<ui>(screen); }
  speed = 77 * kmh;
  hidden = true;
  check(speed.get().value == 77 && hidden.get());
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  before_binding();
  widgets_follow(screen);
  c_code_shares_the_subject();
  observers();
  runtime_binding(screen);
  survives_a_deleted_tree(screen);

  // States outlive LVGL here, so release their subjects first.
  speed.reset();
  hidden.reset();
  title.reset();
  flow.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
