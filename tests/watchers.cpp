#include <lv/lv.hpp>
#include <lv/detail/watch.hpp>

#include "check.hpp"

#include <array>
#include <cstring>
#include <utility>

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

// ---- the list of watchers on its own ----------------------------------------------

using lv::detail::watch_list;
using lv::detail::watch_node;

// A watcher that records its letter, and may do something while it is told.
struct probe : watch_node {
  char who = '?';
  void (*act)(probe& self) = nullptr;
  int told = 0;

  explicit probe(char letter, void (*action)(probe&) = nullptr) : who(letter), act(action) {
    changed = &run;
  }
  static void run(watch_node& n) {
    auto& self = static_cast<probe&>(n);
    ++self.told;
    tell(self.who, 0);
    if (self.act != nullptr) self.act(self);
  }
};

// What the actions below work on.
watch_list* the_list = nullptr;
probe* victim = nullptr;
probe* newcomer = nullptr;
bool acted = false;

// Each action does its work once, so that a nested walk does not repeat it.
void remove_victim(probe&) {
  if (std::exchange(acted, true)) return;
  watch_list::remove(*victim);
}
void remove_self(probe& self) {
  if (std::exchange(acted, true)) return;
  watch_list::remove(self);
}
void add_newcomer(probe&) {
  if (std::exchange(acted, true)) return;
  the_list->append(*newcomer);
}
void tell_again(probe&) {
  if (std::exchange(acted, true)) return;
  the_list->tell();
}
void detach_everything(probe&) {
  if (std::exchange(acted, true)) return;
  the_list->detach_all();
}

void the_list_tells_in_order() {
  watch_list list;
  probe a{'a'}, b{'b'}, c{'c'};
  list.append(a);
  list.append(b);
  list.append(c);
  check(!list.empty() && a.linked() && b.linked() && c.linked());
  reset_log();
  list.tell();
  check(log_is("a0b0c0"));

  // Taking out the middle, the first and the last node leaves the others in order.
  watch_list::remove(b);
  check(!b.linked());
  reset_log();
  list.tell();
  check(log_is("a0c0"));
  watch_list::remove(a);
  reset_log();
  list.tell();
  check(log_is("c0"));
  list.append(a);
  list.append(b);
  watch_list::remove(b);
  reset_log();
  list.tell();
  check(log_is("c0a0"));

  watch_list::remove(b);  // not in a list: nothing happens
  watch_list::remove(a);
  watch_list::remove(c);
  check(list.empty());
  reset_log();
  list.tell();
  check(log_size == 0);
}

// A watcher takes out the one that is next, one further on, and itself.
void a_watcher_removes_others_during_a_walk() {
  watch_list list;
  the_list = &list;
  probe b{'b'}, c{'c'}, d{'d'};
  victim = &b;

  acted = false;
  probe a{'a', &remove_victim};
  list.append(a);
  list.append(b);
  list.append(c);
  reset_log();
  list.tell();
  check(log_is("a0c0") && a.told == 1 && b.told == 0 && !b.linked());  // a is not told twice

  victim = &c;  // the last one
  acted = false;
  list.append(d);
  reset_log();
  list.tell();
  check(log_is("a0d0") && c.told == 1 && !c.linked());

  // The last node takes the walk's end with it: a is told, the walk ends there.
  acted = false;
  victim = &d;
  reset_log();
  list.tell();
  check(log_is("a0"));
  check(!d.linked());

  // A watcher takes out itself: it is told once, the others as usual.
  watch_list second;
  the_list = &second;
  acted = false;
  probe e{'e', &remove_self};
  probe f{'f'};
  second.append(e);
  second.append(f);
  reset_log();
  second.tell();
  check(log_is("e0f0") && !e.linked());
  reset_log();
  second.tell();
  check(log_is("f0"));
  watch_list::remove(f);
  watch_list::remove(a);
}

// A watcher linked during a walk is not told by it, but by the next one.
void a_watcher_added_during_a_walk() {
  watch_list list;
  the_list = &list;
  probe n{'n'};
  newcomer = &n;
  acted = false;
  probe a{'a', &add_newcomer};
  probe b{'b'};
  list.append(a);
  list.append(b);
  reset_log();
  list.tell();
  check(log_is("a0b0") && n.linked());
  reset_log();
  list.tell();
  check(log_is("a0b0n0"));

  // Also when the walk is at its last node.
  watch_list alone;
  the_list = &alone;
  probe m{'m'};
  newcomer = &m;
  acted = false;
  probe z{'z', &add_newcomer};
  alone.append(z);
  reset_log();
  alone.tell();
  check(log_is("z0"));
  watch_list::remove(n);
}

// A walk inside a walk tells everyone; the walk it interrupted ends.
void a_nested_walk() {
  watch_list list;
  the_list = &list;
  acted = false;
  probe a{'a', &tell_again};
  probe b{'b'};
  probe c{'c'};
  list.append(a);
  list.append(b);
  list.append(c);
  reset_log();
  list.tell();
  check(log_is("a0a0b0c0"));
  check(b.told == 1 && c.told == 1);

  // A nested walk of an empty list interrupts nothing.
  watch_list empty;
  empty.tell();
  check(empty.empty());
}

// Taking every node out ends the walk, and leaves the nodes free to join a list again.
void detaching_ends_the_walk() {
  watch_list list;
  the_list = &list;
  acted = false;
  probe a{'a', &detach_everything};
  probe b{'b'};
  list.append(a);
  list.append(b);
  reset_log();
  list.tell();
  check(log_is("a0") && !a.linked() && !b.linked() && list.empty());
  list.append(b);
  reset_log();
  list.tell();
  check(log_is("b0"));
  watch_list::remove(b);
}

// A moved node is in no list, whatever the old one was in.
void a_moved_node_is_free() {
  watch_list list;
  probe a{'a'};
  list.append(a);
  watch_node moved{std::move(static_cast<watch_node&>(a))};
  check(!moved.linked() && moved.changed == a.changed);
  watch_list::remove(a);
}

// ---- a state's watchers -----------------------------------------------------------

std::size_t heap_blocks() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.used_cnt;
}

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// A state that nothing asks for a subject has none: binding makes none, raw() makes it.
static_assert(sizeof(lv::state<lv::counter>) <= 4 * sizeof(void*));

void binding_makes_no_subject(lv::obj screen) {
  lv::state n{lv::counter{1}};
  const auto start = heap_blocks();
  auto label = screen.add<"label">().set({.text = lv::bind(n, "{}")});
  const auto bound = heap_blocks();
  check(bound > start);

  // Changes allocate nothing and leave the heap as it was, even for a longer text.
  const auto used = heap_used();
  const auto blocks = heap_blocks();
  for (int i = 0; i < 50; ++i) n = lv::counter{i * 1000};
  n = lv::counter{1};
  check(heap_used() == used && heap_blocks() == blocks);

  // The subject is made when it is asked for, and not before.
  n.raw();
  check(heap_blocks() > blocks);
  lv_obj_delete(label.raw());
}

// What a state's list gives C code, and what C code gives it back.
int c_calls = 0;
int c_value = -1;
void c_sees(lv_observer_t*, lv_subject_t* s) {
  ++c_calls;
  c_value = lv_subject_get_int(s);
}

void c_code_and_watchers_share_the_value(lv::obj screen) {
  lv::state n{lv::counter{1}};
  auto label = screen.add<"label">().set({.text = lv::bind(n, "n={}")});
  int runs = 0;
  int last = -1;
  lv::effect e{[&runs, &last](lv::counter c) {
                 ++runs;
                 last = c.value;
               },
               n};
  check(e.start() && runs == 1);

  // The subject comes after the watchers. LVGL tells a new observer at once, and nobody
  // else is told for that.
  lv_subject_t* s = n.raw();
  check(runs == 1 && std::strcmp(label.get<"text">(), "n=1") == 0);
  c_calls = 0;
  lv_observer_t* o = lv_subject_add_observer(s, &c_sees, nullptr);
  check(c_calls == 1 && c_value == 1);

  // lvgl26 writes: the watchers and the C observer are told once each.
  n = lv::counter{2};
  check(runs == 2 && last == 2 && c_calls == 2 && c_value == 2);
  check(std::strcmp(label.get<"text">(), "n=2") == 0);

  // C code writes: the watchers are told, and C sees its own write once.
  lv_subject_set_int(s, 9);
  check(runs == 3 && last == 9 && c_calls == 3 && c_value == 9);
  check(std::strcmp(label.get<"text">(), "n=9") == 0);
  check(n.get() == lv::counter{9});

  // The same value from either side tells nobody.
  lv_subject_set_int(s, 9);
  n = lv::counter{9};
  check(runs == 3 && c_calls == 3);

  // The watchers of lvgl26 are told before C observers, whatever the order of adding.
  reset_log();
  lv::effect first{recorder{'e'}, n};
  check(first.start());
  reset_log();
  n = lv::counter{4};
  check(log_is("e4"));

  lv_observer_delete(o);
  lv_obj_delete(label.raw());
}

// The widget and the state may go in either order.
struct level {
  int value;
};

void the_state_goes_first(lv::obj screen) {
  lv_obj_t* arc = nullptr;
  lv_obj_t* label = nullptr;
  {
    lv::state<level> n{level{3}};
    auto a = screen.add<"arc">().set({.range = {0, 100}, .value = lv::bind(n)});
    auto l = screen.add<"label">().set({.text = lv::bind(n, "{}")});
    arc = a.raw();
    label = l.raw();
    check(lv_arc_get_value(arc) == 3);
  }
  // The state is gone: the widgets keep what they showed and follow nothing. A write from
  // the arc finds nobody to tell.
  check(std::strcmp(lv_label_get_text(label), "3") == 0);
  lv_arc_set_value(arc, 40);
  lv_obj_send_event(arc, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_delete(arc);
  lv_obj_delete(label);
}

void the_widget_goes_first(lv::obj screen) {
  lv::state<level> n{level{3}};
  auto a = screen.add<"arc">().set({.range = {0, 100}, .value = lv::bind(n)});
  lv_obj_delete(a.raw());
  n = level{4};
  check(n.get().value == 4);
}

// A watcher that deletes a widget bound to the same state, while the state tells.
void a_widget_deleted_during_a_walk(lv::obj screen) {
  lv::state n{lv::counter{1}};
  lv_obj_t* doomed = nullptr;
  lv::effect killer{[&doomed](lv::counter c) {
                      if (c.value == 2 && doomed != nullptr) lv_obj_delete(std::exchange(doomed, nullptr));
                    },
                    n};
  check(killer.start());  // first in the list
  auto label = screen.add<"label">().set({.text = lv::bind(n, "{}")});
  doomed = label.raw();
  n = lv::counter{2};  // the killer deletes the label before the label is told
  check(doomed == nullptr);
  n = lv::counter{3};
  check(n.get() == lv::counter{3});
}

// An effect that resets the state that it follows ends the walk.
void a_watcher_resets_the_state() {
  lv::state n{lv::counter{1}};
  int later = 0;
  lv::effect first{[&n](lv::counter c) {
                     if (c.value == 2) n.reset();
                   },
                   n};
  lv::effect second{[&later](lv::counter) { ++later; }, n};
  check(first.start() && second.start() && later == 1);
  n = lv::counter{2};
  check(later == 1 && n.get() == lv::counter{2});  // the second was told nothing: nobody follows
  n = lv::counter{3};
  check(later == 1);
}

// A user's change reaches the state, and the widgets that follow it.
void an_arc_writes_back(lv::obj screen) {
  lv::state<level> n{level{3}};
  auto a = screen.add<"arc">().set({.range = {0, 100}, .value = lv::bind(n)});
  auto l = screen.add<"label">().set({.text = lv::bind(n, "{}")});
  lv_arc_set_value(a.raw(), 55);
  lv_obj_send_event(a.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(n.get().value == 55 && std::strcmp(l.get<"text">(), "55") == 0);
  lv_obj_delete(a.raw());
  lv_obj_delete(l.raw());
}

// A watcher that starts an effect during a walk: the effect runs once, when it starts.
void an_effect_started_during_a_walk_runs_once() {
  lv::state n{lv::counter{0}};
  int late_runs = 0;
  lv::effect late{[&late_runs](lv::counter) { ++late_runs; }, n};
  lv::effect starter{[&late](lv::counter c) {
                       if (c.value == 1) (void)late.start();
                     },
                     n};
  check(starter.start() && late_runs == 0);
  n = lv::counter{1};
  check(late_runs == 1);
  n = lv::counter{2};
  check(late_runs == 2);
}

// Widgets and C observers: lvgl26's watchers are told first, whatever the order of adding.
void watchers_are_told_before_c_observers() {
  lv::state n{lv::counter{0}};
  struct local {
    static void c_logs(lv_observer_t*, lv_subject_t* s) { tell('c', lv_subject_get_int(s)); }
  };
  lv_observer_t* o = lv_subject_add_observer(n.raw(), &local::c_logs, nullptr);  // C first
  lv::effect e{recorder{'e'}, n};
  reset_log();
  check(e.start());
  reset_log();
  n = lv::counter{3};
  check(log_is("e3c3"));
  reset_log();
  lv_subject_set_int(n.raw(), 5);
  check(log_is("e5c5"));
  lv_observer_delete(o);
}

// These bindings still follow through LVGL's own bind function, and so make an LVGL
// subject and observer: LVGL animates them once the widget has been drawn, and tells that
// by a private field (keeps_lvgl_binder in lv/props.hpp). The label above does not.
void bar_slider_roller_and_checked_keep_lvgls_observer(lv::obj screen) {
  const auto extra_blocks_by_raw = [](auto& source) {
    const auto before = heap_blocks();
    source.raw();
    return heap_blocks() - before;
  };

  lv::state<level> bar_value{level{1}};
  auto bar = screen.add<"bar">().set({.value = lv::bind(bar_value)});
  check(extra_blocks_by_raw(bar_value) == 0);  // the subject was made by the binding

  lv::state<level> slider_value{level{1}};
  auto slider = screen.add<"slider">().set({.value = lv::bind(slider_value)});
  check(extra_blocks_by_raw(slider_value) == 0);

  lv::state<level> roller_value{level{1}};
  auto roller = screen.add<"roller">().set({.selected = lv::bind(roller_value)});
  check(extra_blocks_by_raw(roller_value) == 0);

  lv::state<bool> flag{false};
  auto box = screen.add<"checkbox">().set({.checked = lv::bind(flag)});
  check(extra_blocks_by_raw(flag) == 0);
  lv_obj_add_state(box.raw(), LV_STATE_CHECKED);
  lv_obj_send_event(box.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(flag.get());  // and the user's change comes back

  // An arc and a label follow through a watcher: the subject comes with raw().
  lv::state<level> arc_value{level{1}};
  auto arc = screen.add<"arc">().set({.value = lv::bind(arc_value)});
  check(extra_blocks_by_raw(arc_value) > 0);

  lv_obj_delete(bar.raw());
  lv_obj_delete(slider.raw());
  lv_obj_delete(roller.raw());
  lv_obj_delete(box.raw());
  lv_obj_delete(arc.raw());
}

// A widget that the user changes writes to a state, and to nothing that is computed.
void a_bound_arc_does_not_write_to_a_computed(lv::obj screen) {
  lv::state<level> n{level{3}};
  lv::computed same{[](level l) { return l; }, n};
  auto arc = screen.add<"arc">().set({.range = {0, 100}, .value = lv::bind(same)});
  check(lv_arc_get_value(arc.raw()) == 3);
  lv_arc_set_value(arc.raw(), 70);
  lv_obj_send_event(arc.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(n.get().value == 3 && same.get().value == 3);
  n = level{8};
  check(lv_arc_get_value(arc.raw()) == 8);
  lv_obj_delete(arc.raw());
}

void all(lv::obj screen) {
  told_in_order();
  equal_values_are_not_passed_on();
  a_nested_change();
  a_bound_label_follows(screen);
  the_list_tells_in_order();
  a_watcher_removes_others_during_a_walk();
  a_watcher_added_during_a_walk();
  a_nested_walk();
  detaching_ends_the_walk();
  a_moved_node_is_free();
  binding_makes_no_subject(screen);
  c_code_and_watchers_share_the_value(screen);
  the_state_goes_first(screen);
  the_widget_goes_first(screen);
  a_widget_deleted_during_a_walk(screen);
  a_watcher_resets_the_state();
  an_arc_writes_back(screen);
  an_effect_started_during_a_walk_runs_once();
  watchers_are_told_before_c_observers();
  bar_slider_roller_and_checked_keep_lvgls_observer(screen);
  a_bound_arc_does_not_write_to_a_computed(screen);
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
