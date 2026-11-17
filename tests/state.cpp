#include <lv/lv.hpp>

#include <mp-units/systems/international.h>
#include <mp-units/systems/si.h>

#include "check.hpp"

#include <cstring>
#include <string_view>

namespace {

using namespace lv::units;

using namespace mp_units::si::unit_symbols;

// Measures are mp-units quantities. A unit of your own is defined the mp-units way.
inline constexpr struct rpm final : mp_units::named_unit<"rpm", mp_units::one / mp_units::si::minute> {
} rpm;
using speed_t = mp_units::quantity<mp_units::si::kilo<mp_units::si::metre> / mp_units::si::hour, int>;
static_assert(lv::detail::integer_quantity<speed_t>);
// A text binding shows a quantity with its unit; a percent sign in the unit is escaped.
static_assert(std::string_view(lv::detail::quantity_format<speed_t>) == "%d km/h");
static_assert(std::string_view(
                  lv::detail::quantity_format<mp_units::quantity<mp_units::percent, int>>) == "%d %%");
static_assert(!lv::detail::unit_quantity<int> && !lv::detail::unit_quantity<lv::angle>);

// A count or an index is a struct with one member.
struct trip_id {
  std::uint16_t value;
};
static_assert(lv::detail::strong_value<trip_id> && lv::detail::strong_value<lv::angle>);
static_assert(!lv::detail::strong_value<int> && !lv::detail::strong_value<lv_color_t>);

// Constant-initialized globals: no subject exists until something binds to them.
constinit lv::state speed{40 * km / h};
static_assert(std::is_same_v<decltype(speed), lv::state<speed_t>>);
constinit lv::state revs{800 * rpm};
constinit lv::state<bool> hidden{false};
constinit lv::state<const char*> title{"Trip"};
constinit lv::state<lv_flex_flow_t> flow{LV_FLEX_FLOW_COLUMN};

constexpr auto ui = lv::column({.flex_flow = lv::bind(flow), .size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = lv::bind(title)}),
    lv::label({.id = "reading", .text = lv::bind(speed)}),  // shown with its unit
    lv::label({.id = "custom", .text = lv::bind(speed, "now {}")}),
    lv::label({.id = "revs", .text = lv::bind(revs)}),
    lv::arc({.id = "dial", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::bar({.id = "bar", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::label({.id = "extra", .text = "x", .user_data = nullptr})];

int c_side_value = -1;
void c_observer(lv_observer_t*, lv_subject_t* s) { c_side_value = lv_subject_get_int(s); }

void counters(lv::obj screen) {
  static constinit lv::state clicks{lv::counter{}};
  static_assert(lv::detail::strong_value<lv::counter>);

  auto l = screen.add<"label">().set({.text = lv::bind(clicks, "{} clicks")});
  ++clicks;
  clicks += 4;
  check(clicks.get() == lv::counter{5} && clicks.number() == 5);
  --clicks;
  clicks -= 1;
  clicks.update([](lv::counter c) { return c += 2; });
  check(clicks.number() == 5);
  check(std::strcmp(l.get<"text">(), "5 clicks") == 0);

  lv_obj_delete(l.raw());
  clicks.reset();
}

void before_binding() {
  // A state works as a plain value before any subject exists.
  lv::state<trip_id> local{trip_id{1}};
  local = trip_id{2};
  check(local.get().value == 2);

  // Releasing a state that has no subject keeps its value.
  local.reset();
  check(local.get().value == 2);
}

// A bool is read from the subject as "not zero", whatever C code wrote there.
void bool_through_a_subject() {
  lv::state<bool> on{false};
  lv_subject_t* s = on.raw();
  check(!on.get());
  on = true;
  check(on.get() && lv_subject_get_int(s) == 1);
  lv_subject_set_int(s, -1);
  check(on.get());
  lv_subject_set_int(s, 0);
  check(!on.get());
}

// reset() keeps what C code wrote last and deletes the subject; the next use makes a new one.
void reset_keeps_the_value() {
  lv::state<trip_id> local{trip_id{1}};
  lv_subject_set_int(local.raw(), 8);
  static int seen = 0;
  local.observe([](trip_id t) { seen = t.value; });
  local.reset();
  check(local.get().value == 8);
  local = trip_id{9};
  check(seen == 8);  // the observer went with the old subject
  check(lv_subject_get_int(local.raw()) == 9);
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
  check(std::strcmp(root->get<"revs">().get<"text">(), "800 rpm") == 0);

  // mp-units converts where nothing is lost: 2 km/min is 120 km/h.
  speed = 2 * km / min;
  check(dial.get<"value">() == 120);
  check(std::strcmp(reading.get<"text">(), "120 km/h") == 0);
  const auto mph = speed.get().in<double>(mp_units::international::mile / h);
  check(mph.numerical_value_in(mp_units::international::mile / h) > 74.5 &&
        mph.numerical_value_in(mp_units::international::mile / h) < 74.6);
  speed = 40 * km / h;
  speed += 10 * km / h;
  check(speed.number() == 50);
  speed -= 10 * km / h;
  check(std::strcmp(root->get<"title">().get<"text">(), "Trip") == 0);

  speed = 120 * km / h;
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
  check(speed.get().numerical_value_in(km / h) == 75);
  check(root->get<"bar">().get<"value">() == 75);
}

// LVGL names the bind function of a roller and of a dropdown lv_<widget>_bind_value,
// and their setter lv_<widget>_set_selected. A bind function that matches no setter
// belongs to the only property the widget sets and gets as a plain integer.
static_assert(lv::detail::binder_of(^^lv_roller_set_selected) == ^^lv_roller_bind_value);
static_assert(lv::detail::binder_of(^^lv_dropdown_set_selected) == ^^lv_dropdown_bind_value);
// No getter: the row count is not the roller's value.
static_assert(lv::detail::binder_of(^^lv_roller_set_visible_row_count) == std::meta::info{});
// Not an integer: a text, a bool and an enum.
static_assert(lv::detail::binder_of(^^lv_roller_set_options) == std::meta::info{});
static_assert(lv::detail::binder_of(^^lv_dropdown_set_selected_highlight) == std::meta::info{});
static_assert(lv::detail::binder_of(^^lv_dropdown_set_dir) == std::meta::info{});
// A name that matches still comes first.
static_assert(lv::detail::binder_of(^^lv_arc_set_value) == ^^lv_arc_bind_value);
static_assert(lv::detail::binder_of(^^lv_slider_set_value) == ^^lv_slider_bind_value);
// obj's bind functions all match a setter or take more than a subject.
static_assert(lv::detail::binder_of(^^lv_obj_set_checked) == ^^lv_obj_bind_checked);
static_assert(lv::detail::binder_of(^^lv_obj_set_x) == std::meta::info{});
// Several integer properties that set and get: none of them is the one a binder means.
static_assert(lv::detail::only_integer_setter("lv_obj_") == std::meta::info{});
static_assert(lv::detail::only_integer_setter("lv_roller_") == ^^lv_roller_set_selected);

struct choice {
  std::int32_t value;
};
constinit lv::state<choice> weekday{choice{1}};

constexpr auto week = lv::roller({.id = "week", .options = {"Mon\nTue\nWed", "normal"},
                                  .selected = lv::bind(weekday)});

// A roller and a dropdown follow a state, and write the user's choice back to it.
void choices_follow_a_state(lv::obj screen) {
  auto root = lv::mount<week>(screen);
  check(root.has_value());
  auto roller = root->get<"week">();
  check(lv_roller_get_selected(roller.raw()) == 1);
  weekday = choice{2};
  check(lv_roller_get_selected(roller.raw()) == 2);
  lv_roller_set_selected(roller.raw(), 0, LV_ANIM_OFF);
  lv_obj_send_event(roller.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(weekday.get().value == 0);

  lv::state<choice> pick{choice{2}};
  auto dropdown = screen.add<"dropdown">().set({.options = "A\nB\nC", .selected = lv::bind(pick)});
  check(lv_dropdown_get_selected(dropdown.raw()) == 2);
  pick = choice{1};
  check(lv_dropdown_get_selected(dropdown.raw()) == 1);
  lv_dropdown_set_selected(dropdown.raw(), 0);
  lv_obj_send_event(dropdown.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(pick.get().value == 0);
  lv_obj_delete(dropdown.raw());
}

// mp-units has a `deg` of its own, so lvgl26's is written lv::deg in this file.
// An angle follows a state of its own kind: tenths of a degree for an image, whole
// degrees for an arc. (A state of lv::angle on the image's rotation does not
// compile: see tests/fail/bind_angle_to_decidegrees.cpp.)
void angles_follow_a_state(lv::obj screen) {
  lv::state turn{22.5 * lv::deg};
  static_assert(std::same_as<decltype(turn), lv::state<lv::decidegrees>>);
  auto image = screen.add<"image">().set({.rotation = lv::bind(turn)});
  check(lv_image_get_rotation(image.raw()) == 225);
  turn = 90 * lv::deg;
  check(lv_image_get_rotation(image.raw()) == 900);
  check(turn.get() == 90 * lv::deg && turn.number() == 900);
  lv_obj_delete(image.raw());

  lv::state quarter{90 * lv::deg};
  auto arc = screen.add<"arc">().set({.rotation = lv::bind(quarter)});
  check(lv_arc_get_rotation(arc.raw()) == 90);
  quarter = 180 * lv::deg;
  check(lv_arc_get_rotation(arc.raw()) == 180);
  lv_obj_delete(arc.raw());
}

void c_code_shares_the_subject() {
  lv_observer_t* o = lv_subject_add_observer(speed.raw(), &c_observer, nullptr);
  speed = 33 * km / h;
  check(c_side_value == 33);
  lv_subject_set_int(speed.raw(), 44);  // C writes, C++ reads
  check(speed.get().numerical_value_in(km / h) == 44);
  lv_observer_delete(o);
}

void observers() {
  static int seen = -1;
  lv_observer_t* a = speed.observe([](speed_t v) { seen = v.numerical_value_in(km / h); });
  check(seen == speed.get().numerical_value_in(km / h));  // called once right away
  speed = 9 * km / h;
  check(seen == 9);

  int captured = -1;
  lv_observer_t* b = speed.observe([&captured](speed_t v) { captured = v.numerical_value_in(km / h); });
  speed = 10 * km / h;
  check(captured == 10);
  lv_observer_delete(a);
  lv_observer_delete(b);
  speed = 11 * km / h;
  check(seen == 10 && captured == 10);
}

// A state that is a local object: its address is not a constant, so the binding is
// made at run time and not folded into the program by the compiler.
void binding_to_a_local_state(lv::obj screen) {
  lv::state local{lv::counter{2}};
  auto l = screen.add<"label">().set({.text = lv::bind(local, "n={}")});
  check(std::strcmp(l.get<"text">(), "n=2") == 0);
  ++local;
  check(std::strcmp(l.get<"text">(), "n=3") == 0);
  lv_obj_delete(l.raw());
}

void runtime_binding(lv::obj screen) {
  auto l = screen.add<"label">().set({.text = lv::bind(speed, "{}")});
  speed = 5 * km / h;
  check(std::strcmp(l.get<"text">(), "5") == 0);

  // Deleting the widget removes its observer: setting the state must be safe.
  lv_obj_delete(l.raw());
  speed = 6 * km / h;
  check(speed.get().numerical_value_in(km / h) == 6);
}

void survives_a_deleted_tree(lv::obj screen) {
  { auto root = lv::mount<ui>(screen); }
  speed = 77 * km / h;
  hidden = true;
  check(speed.get().numerical_value_in(km / h) == 77 && hidden.get());
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  before_binding();
  bool_through_a_subject();
  reset_keeps_the_value();
  counters(screen);
  widgets_follow(screen);
  c_code_shares_the_subject();
  observers();
  runtime_binding(screen);
  angles_follow_a_state(screen);
  choices_follow_a_state(screen);
  binding_to_a_local_state(screen);
  survives_a_deleted_tree(screen);

  // States outlive LVGL here, so release their subjects first.
  speed.reset();
  revs.reset();
  hidden.reset();
  title.reset();
  flow.reset();
  weekday.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
