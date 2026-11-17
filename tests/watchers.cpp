#include <lv/lv.hpp>

#include "check.hpp"

#include <array>
#include <cstring>

// What a state's watchers see: the order they are told in, which changes tell them, and
// what happens when a watcher changes the state or its neighbours while it is told. The
// watchers here are lv::effect objects and bound widgets: the two ways to follow a state
// through the public API. include/lv/state.hpp says what is promised.

namespace {

// What the watchers were told: a letter and the number, in the order of the calls.
std::array<char, 64> log_text{};
std::size_t log_size = 0;

void reset_log() {
  log_text.fill('\0');
  log_size = 0;
}

void tell(char who, int value) {
  if (log_size + 2 < log_text.size()) {
    log_text[log_size++] = who;
    log_text[log_size++] = static_cast<char>('0' + value);
  }
}

bool log_is(const char* expected) { return std::strcmp(log_text.data(), expected) == 0; }

// Each effect records its own letter. They are started in the order of the letters.
struct recorder {
  char who;
  void operator()(lv::counter c) const { tell(who, c.value); }
};

// Watchers are told in the order they started following.
void told_in_order() {
  lv::state n{lv::counter{0}};
  lv::effect a{recorder{'a'}, n};
  lv::effect b{recorder{'b'}, n};
  lv::effect c{recorder{'c'}, n};
  reset_log();
  check(a.start() && b.start() && c.start());
  check(log_is("a0b0c0"));  // each runs once when it starts

  reset_log();
  n = lv::counter{1};
  check(log_is("a1b1c1"));

  // The order is the order of starting, not of the objects: b follows again after c.
  b.reset();
  check(b.start());
  reset_log();
  n = lv::counter{2};
  check(log_is("a2c2b2"));
}

// Setting a value that is there already tells nobody; a pointer is always passed on.
void equal_values_are_not_passed_on() {
  lv::state n{lv::counter{3}};
  int runs = 0;
  lv::effect e{[&runs](lv::counter) { ++runs; }, n};
  check(e.start() && runs == 1);
  n = lv::counter{3};
  check(runs == 1);
  n = lv::counter{4};
  check(runs == 2);

  static constexpr const char* text = "same";
  lv::state<const char*> pointer{text};
  int pointer_runs = 0;
  lv::effect p{[&pointer_runs](const char*) { ++pointer_runs; }, pointer};
  check(p.start() && pointer_runs == 1);
  pointer = text;  // LVGL cannot compare what a pointer points at
  check(pointer_runs == 2);
}

// A watcher that changes the state tells everyone the new value at once; the walk it
// was in does not tell them again with a value that is no longer current.
void a_nested_change() {
  lv::state n{lv::counter{0}};
  lv::effect a{[&n](lv::counter c) {
                 tell('a', c.value);
                 if (c.value == 1) n = lv::counter{2};
               },
               n};
  lv::effect b{recorder{'b'}, n};
  lv::effect c{recorder{'c'}, n};
  check(a.start() && b.start() && c.start());

  reset_log();
  n = lv::counter{1};
  // a sees 1 and sets 2: the nested walk tells a, b and c about 2. The walk of 1 ends there.
  check(log_is("a1a2b2c2"));
  check(n.get() == lv::counter{2});
}

// A bound label shows the value at once, and follows later changes in the same order.
void a_bound_label_follows(lv::obj screen) {
  lv::state n{lv::counter{5}};
  auto first = screen.add<"label">().set({.text = lv::bind(n, "a{}")});
  check(std::strcmp(first.get<"text">(), "a5") == 0);
  n = lv::counter{6};
  check(std::strcmp(first.get<"text">(), "a6") == 0);
  lv_obj_delete(first.raw());
  n = lv::counter{7};
  check(n.get() == lv::counter{7});
}

void all(lv::obj screen) {
  told_in_order();
  equal_values_are_not_passed_on();
  a_nested_change();
  a_bound_label_follows(screen);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  all(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
