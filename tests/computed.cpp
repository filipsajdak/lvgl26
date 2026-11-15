#include <lv/lv.hpp>

#include <mp-units/systems/si.h>

#include "check.hpp"

#include <array>
#include <cstring>
#include <type_traits>

namespace {

using namespace lv::units;

using namespace mp_units::si::unit_symbols;

using speed_t = mp_units::quantity<mp_units::si::kilo<mp_units::si::metre> / mp_units::si::hour, int>;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

bool is_hidden(lv::obj o) { return lv_obj_is_hidden(o.raw()); }

constinit lv::state speed{40 * km / h};
constinit lv::state limit{50 * km / h};

// Constant-initialized globals: the dependencies are part of the object.
constinit lv::computed over_limit{[](speed_t s, speed_t l) { return s > l; }, speed, limit};
static_assert(std::is_same_v<decltype(over_limit)::value_type, bool>);

// A quantity made from two quantities, by mp-units' rules.
constinit lv::computed gap{[](const speed_t& s, const speed_t& l) { return s - l; }, speed, limit};
static_assert(std::is_same_v<decltype(gap)::value_type, speed_t>);

// A computed of a computed. An enum and a text are results too.
constinit lv::computed flow{
    [](bool over) { return over ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN; }, over_limit};
constinit lv::computed advice{
    [](bool over) -> const char* { return over ? "Slow down" : "Fine"; }, over_limit};

// A named value, from an ordinary function.
struct bars {
  int value;
};
bars bars_of(speed_t s) { return bars{s.numerical_value_in(km / h) / 10}; }
constinit lv::computed level{&bars_of, speed};
static_assert(std::is_same_v<decltype(level)::value_type, bars>);

// A const computed can be bound and released too.
constinit const lv::computed tint{
    [](bool over) { return over ? lv_color_hex(0xff0000) : lv_color_hex(0x00ff00); }, over_limit};

static_assert(lv::detail::dependency<lv::state<bool>> && lv::detail::dependency<decltype(gap)>);
static_assert(!lv::detail::dependency<bool> && !lv::detail::dependency<lv::state<bool>*>);

constexpr auto ui = lv::column({.flex_flow = lv::bind(flow), .size = {320 * px, 240 * px}})[
    lv::label({.id = "advice", .text = lv::bind(advice)}),
    lv::label({.hidden = lv::bind(over_limit), .id = "warning", .text = "!"}),
    lv::label({.id = "gap", .text = lv::bind(gap)}),  // shown with its unit
    lv::label({.id = "gap_text", .text = lv::bind(gap, "{} over")}),
    lv::bar({.id = "bar", .range = {-240, 240}, .value = lv::bind(gap)}),
    lv::bar({.id = "level", .range = {0, 24}, .value = lv::bind(level)})];

void start_values() {
  speed = 40 * km / h;
  limit = 50 * km / h;
}

void release_globals() {
  // In an order that is not the order of the dependencies: every order is safe.
  speed.reset();
  flow.reset();
  over_limit.reset();
  gap.reset();
  advice.reset();
  level.reset();
  tint.reset();
  limit.reset();
}

// A computed that nothing is bound to is a function call: no subject, no heap.
void works_without_a_subject() {
  start_values();
  const auto before = free_bytes();
  check(!over_limit.get());
  check(gap.get() == -10 * km / h);
  check(level.get().value == 4);
  check(std::strcmp(advice.get(), "Fine") == 0);

  speed = 72 * km / h;
  check(over_limit.get());
  check(gap.get() == 22 * km / h);
  check(level.get().value == 7);
  check(flow.get() == LV_FLEX_FLOW_ROW);
  check(std::strcmp(advice.get(), "Slow down") == 0);
  check(lv_color_eq(tint.get(), lv_color_hex(0xff0000)));
  check(free_bytes() == before);

  // Releasing a computed that has no subject does nothing.
  over_limit.reset();
  check(over_limit.get());
}

void widgets_follow(lv::obj screen) {
  start_values();
  auto root = lv::mount<ui>(screen);
  check(root.has_value());
  if (!root) return;

  auto warning = root->get<"warning">();
  auto gap_label = root->get<"gap">();
  check(!is_hidden(warning));
  check(std::strcmp(gap_label.get<"text">(), "-10 km/h") == 0);
  check(std::strcmp(root->get<"gap_text">().get<"text">(), "-10 over") == 0);
  check(std::strcmp(root->get<"advice">().get<"text">(), "Fine") == 0);
  check(root->get<"bar">().get<"value">() == -10);
  check(root->get<"level">().get<"value">() == 4);
  check(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);

  // One dependency changes.
  speed = 95 * km / h;
  check(is_hidden(warning));
  check(std::strcmp(gap_label.get<"text">(), "45 km/h") == 0);
  check(std::strcmp(root->get<"gap_text">().get<"text">(), "45 over") == 0);
  check(std::strcmp(root->get<"advice">().get<"text">(), "Slow down") == 0);
  check(root->get<"bar">().get<"value">() == 45);
  check(root->get<"level">().get<"value">() == 9);
  check(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_ROW);

  // Then the other.
  limit = 120 * km / h;
  check(!is_hidden(warning));
  check(std::strcmp(gap_label.get<"text">(), "-25 km/h") == 0);
  check(std::strcmp(root->get<"advice">().get<"text">(), "Fine") == 0);
  check(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);

  // A state changed by C code is followed too.
  lv_subject_set_int(speed.raw(), 130);
  check(is_hidden(warning) && gap.get() == 10 * km / h);
  check(std::strcmp(gap_label.get<"text">(), "10 km/h") == 0);
}

int c_side_value = -1;
int c_side_calls = 0;
void c_observer(lv_observer_t*, lv_subject_t* s) {
  c_side_value = lv_subject_get_int(s);
  ++c_side_calls;
}

void c_code_shares_the_subject() {
  start_values();
  lv_subject_t* s = gap.raw();
  check(s != nullptr && gap.raw() == s);  // one subject, made once
  check(lv_subject_get_int(s) == -10);

  c_side_calls = 0;
  lv_observer_t* o = lv_subject_add_observer(s, &c_observer, nullptr);
  check(c_side_value == -10 && c_side_calls == 1);  // LVGL calls a new observer once
  speed = 44 * km / h;
  check(c_side_value == -6 && c_side_calls == 2);

  // A change that leaves the result as it was is not passed on.
  speed = 45 * km / h;
  limit = 51 * km / h;
  check(c_side_value == -6 && c_side_calls == 4);
  lv_subject_set_int(limit.raw(), 51);
  check(c_side_calls == 4);

  // The subject is an output: what C code writes there does not change the computed,
  // and the next change replaces it.
  lv_subject_set_int(s, 99);
  check(gap.get() == -6 * km / h);
  speed = 50 * km / h;
  check(lv_subject_get_int(s) == -1);

  check(lv_color_eq(lv_subject_get_color(tint.raw()), lv_color_hex(0x00ff00)));
  speed = 52 * km / h;
  check(lv_color_eq(lv_subject_get_color(tint.raw()), lv_color_hex(0xff0000)));
  lv_observer_delete(o);
}

// What an observer of `total` was told, and what an effect saw.
std::array<int, 8> told{};
std::size_t told_count = 0;
void record(lv_observer_t*, lv_subject_t* s) {
  if (told_count < told.size()) told[told_count++] = lv_subject_get_int(s);
}

// Two computeds of one state, and a computed of both: a diamond.
void a_diamond_has_no_half_updated_value() {
  lv::state n{lv::counter{1}};
  lv::computed twice{[](lv::counter c) { return lv::counter{c.value * 2}; }, n};
  lv::computed next{[](lv::counter c) { return lv::counter{c.value + 1}; }, n};
  lv::computed total{[](lv::counter a, lv::counter b) { return lv::counter{a.value + b.value}; },
                     twice, next};
  check(total.get() == lv::counter{4});

  told_count = 0;
  lv_subject_add_observer(total.raw(), &record, nullptr);
  int runs = 0;
  bool consistent = true;
  lv::effect watch{[&](lv::counter a, lv::counter b) {
                     ++runs;
                     consistent = consistent && a.value == 2 * (b.value - 1);
                   },
                   twice, next};
  check(watch.start() && runs == 1);

  n = lv::counter{5};
  // 3n+1 = 16. Never 12 (new `twice`, old `next`) and never told twice.
  check(total.get() == lv::counter{16});
  check(told_count == 2 && told[0] == 4 && told[1] == 16);
  // The effect ran once for each of its two dependencies, both times with final values.
  check(runs == 3 && consistent);

  ++n;
  check(told_count == 3 && told[2] == 19);
  check(runs == 5 && consistent);
}

int logged = -1;
constinit lv::effect log_speed{[](speed_t s) { logged = s.numerical_value_in(km / h); }, speed};

void effects() {
  start_values();

  // A constant-initialized effect does nothing until it is started.
  speed = 41 * km / h;
  check(logged == -1);
  check(log_speed.start() && logged == 41);
  speed = 42 * km / h;
  check(logged == 42);
  log_speed.reset();
  speed = 43 * km / h;
  check(logged == 42);
  logged = -1;

  // The function is kept in the effect, so it may capture more than a pointer.
  int runs = 0;
  int last_speed = 0;
  bool last_over = false;
  {
    lv::effect e{[&runs, &last_speed, &last_over](speed_t s, bool over) {
                   ++runs;
                   last_speed = s.numerical_value_in(km / h);
                   last_over = over;
                 },
                 speed, over_limit};
    speed = 44 * km / h;
    check(runs == 0);  // not started yet

    check(e.start());
    check(runs == 1 && last_speed == 44 && !last_over);
    check(e.start() && runs == 1);  // already running

    speed = 45 * km / h;
    check(runs == 2 && last_speed == 45 && !last_over);
    speed = 45 * km / h;  // no change
    check(runs == 2);

    // Past the limit both dependencies change, so it runs twice with the final values.
    speed = 60 * km / h;
    check(runs == 4 && last_speed == 60 && last_over);
    limit = 61 * km / h;
    check(runs == 5 && !last_over);

    e.reset();
    speed = 46 * km / h;
    check(runs == 5);
    e.reset();  // twice is fine

    check(e.start());
    check(runs == 6 && last_speed == 46);
    speed = 47 * km / h;
    check(runs == 7);
  }
  // The destructor stopped it.
  speed = 48 * km / h;
  check(runs == 7);
}

// reset() of a dependency takes the observers on its subject with it.
void a_dependency_is_reset_first() {
  lv::state n{lv::counter{1}};
  const lv::computed twice{[](lv::counter c) { return lv::counter{c.value * 2}; }, n};
  int runs = 0;
  lv::effect e{[&runs](lv::counter, lv::counter) { ++runs; }, n, twice};

  lv_subject_t* s = twice.raw();
  check(e.start() && runs == 1);
  n = lv::counter{2};
  check(lv_subject_get_int(s) == 4 && runs == 3);

  n.reset();
  n = lv::counter{3};
  check(twice.get() == lv::counter{6});  // get() is still right
  check(lv_subject_get_int(s) == 4);     // the subject no longer follows
  check(runs == 3);
  lv_subject_t* again = n.raw();         // a new subject, perhaps at the old address
  check(again != nullptr);

  // Releasing what depended on it is safe, in either order, and more than once.
  twice.reset();
  e.reset();
  twice.reset();

  // Bound and started again, they follow the new subject.
  check(lv_subject_get_int(twice.raw()) == 6);
  check(e.start() && runs == 4);
  n = lv::counter{4};
  check(lv_subject_get_int(twice.raw()) == 8 && runs == 6);

  // Here the dependents go first, as destructors of locals do.
  e.reset();
  twice.reset();
  n = lv::counter{5};
  check(runs == 6 && twice.get() == lv::counter{10});
}

// A computed that is a local object, with a function that captures.
void a_local_computed(lv::obj screen) {
  lv::state local{lv::counter{2}};
  int step = 10;
  lv::computed scaled{[&step](lv::counter c) { return lv::counter{c.value * step}; }, local};
  auto l = screen.add<"label">().set({.text = lv::bind(scaled, "n={}")});
  check(std::strcmp(l.get<"text">(), "n=20") == 0);
  ++local;
  check(std::strcmp(l.get<"text">(), "n=30") == 0);

  // Deleting the widget removes its observer: a change must be safe.
  lv_obj_delete(l.raw());
  ++local;
  check(scaled.get() == lv::counter{40});
}

void all(lv::obj screen) {
  works_without_a_subject();
  widgets_follow(screen);
  c_code_shares_the_subject();
  a_diamond_has_no_half_updated_value();
  effects();
  a_dependency_is_reset_first();
  a_local_computed(screen);
  release_globals();
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  all(screen);

  // Everything once more: subjects and observers are all given back.
  const auto before = free_bytes();
  all(screen);
  check(free_bytes() == before);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
