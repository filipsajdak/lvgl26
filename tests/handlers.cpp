// A captureless lambda as an event handler of a constexpr tree: written in the tree, passed
// through a part that constrains it with lv::tree_handler, and in a part whose state is a
// template argument. A capturing lambda is refused (tests/fail/handler_capturing_lambda.cpp).

#include <lv/lv.hpp>

#include "check.hpp"

#include <type_traits>

namespace {

using namespace lv::units;

int taps = 0;
int seen_code = 0;
constinit lv::state<lv::counter> taps_counted{lv::counter{}};

// A part that takes any handler of an event, as the page docs/concepts.md shows.
constexpr auto tap_button(lv::tree_handler auto clicked) {
  return lv::button({.id = "tap", .on_clicked = clicked})[lv::label({.text = "Tap"})];
}

// A part whose state is a template argument: its handlers need no capture.
template <lv::dependency auto& Count>
constexpr auto counting_button() {
  return lv::button({.id = "count", .on_clicked = [] { Count.update([](lv::counter c) { return ++c; }); }})[
      lv::label({.text = lv::bind(Count, "{}")})];
}

// The part of the README: handlers through lv::tree_handler, a state by reference.
constinit lv::state<lv::counter> copies{lv::counter{5}};
constexpr auto spin_box(const char* name, const lv::dependency auto& count, lv::tree_handler auto fewer,
                        lv::tree_handler auto more) {
  return lv::row[lv::label({.text = name}), lv::button({.id = "fewer", .on_clicked = fewer}),
                 lv::label({.text = lv::bind(count, "{}")}), lv::button({.id = "more", .on_clicked = more})];
}
constexpr auto spun = spin_box("Copies", copies, [] { --copies; }, [] { ++copies; });

constexpr auto in_the_tree = lv::button({.id = "direct", .on_clicked = [] { ++taps; }});
constexpr auto with_the_event = lv::button({.id = "event", .on_pressed = [](lv::event e) { seen_code = e.code(); }});
constexpr auto through_a_part = tap_button([] { ++taps; });
constexpr auto function_pointer = tap_button(+[] { ++taps; });
constexpr auto by_a_state = counting_button<taps_counted>();

constexpr auto all = lv::column({})[in_the_tree, with_the_event, through_a_part, by_a_state, spun];

// The handler is a plain function pointer in the tree: no allocation, nothing stored.
static_assert(std::is_same_v<decltype(in_the_tree.p.on_clicked), lv::arg<lv::handler>>);

void fires(lv::obj screen) {
  auto root = lv::mount<all>(screen);
  check(root.has_value());
  if (!root) return;

  taps = 0;
  lv_obj_send_event(root->get<"direct">().raw(), LV_EVENT_CLICKED, nullptr);
  check(taps == 1);
  lv_obj_send_event(root->get<"tap">().raw(), LV_EVENT_CLICKED, nullptr);
  check(taps == 2);

  lv_obj_send_event(root->get<"event">().raw(), LV_EVENT_PRESSED, nullptr);
  check(seen_code == LV_EVENT_PRESSED);

  taps_counted = lv::counter{};
  lv_obj_send_event(root->get<"count">().raw(), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(root->get<"count">().raw(), LV_EVENT_CLICKED, nullptr);
  check(taps_counted.get() == lv::counter{2});
  taps_counted.reset();

  lv_obj_send_event(root->get<"more">().raw(), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(root->get<"more">().raw(), LV_EVENT_CLICKED, nullptr);
  lv_obj_send_event(root->get<"fewer">().raw(), LV_EVENT_CLICKED, nullptr);
  check(copies.get() == lv::counter{6});
  copies.reset();
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  fires(lv::screen_active());
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
