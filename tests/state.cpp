#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

using namespace lv::literals;

// Constant-initialized globals: no subject exists until something binds to them.
constinit lv::state<int> speed{40};
constinit lv::state<bool> hidden{false};
constinit lv::state<const char*> title{"Trip"};
constinit lv::state<lv_flex_flow_t> flow{LV_FLEX_FLOW_COLUMN};

constexpr auto ui = lv::column({.flex_flow = lv::bind(flow), .size = {320_px, 240_px}})[
    lv::label({.id = "title", .text = lv::bind(title)}),
    lv::label({.id = "reading", .text = lv::bind(speed, "%d km/h")}),
    lv::arc({.id = "dial", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::bar({.id = "bar", .range = {0, 240}, .value = lv::bind(speed)}),
    lv::label({.id = "extra", .text = "x", .user_data = nullptr})];

int c_side_value = -1;
void c_observer(lv_observer_t*, lv_subject_t* s) { c_side_value = lv_subject_get_int(s); }

void before_binding() {
  // A state works as a plain value before any subject exists.
  lv::state<int> local{1};
  local = 2;
  CHECK(local.get() == 2);
}

void widgets_follow(lv::obj screen) {
  auto root = lv::mount<ui>(screen);
  CHECK(root.has_value());
  if (!root) return;

  auto dial = root->get<"dial">();
  auto reading = root->get<"reading">();
  CHECK(dial.get<"value">() == 40);
  CHECK(root->get<"bar">().get<"value">() == 40);
  CHECK(std::strcmp(reading.get<"text">(), "40 km/h") == 0);
  CHECK(std::strcmp(root->get<"title">().get<"text">(), "Trip") == 0);

  speed = 120;
  CHECK(dial.get<"value">() == 120);
  CHECK(root->get<"bar">().get<"value">() == 120);
  CHECK(std::strcmp(reading.get<"text">(), "120 km/h") == 0);

  title = "Home";
  CHECK(std::strcmp(root->get<"title">().get<"text">(), "Home") == 0);

  // A property with no LVGL bind function follows through a generic observer.
  CHECK(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN);
  flow = LV_FLEX_FLOW_ROW;
  CHECK(root->root().get<"style_flex_flow">(LV_PART_MAIN) == LV_FLEX_FLOW_ROW);
  flow = LV_FLEX_FLOW_COLUMN;

  // The arc writes back: a user turning it changes the state.
  dial.set<"value">(75);
  lv_obj_send_event(dial.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  CHECK(speed.get() == 75);
  CHECK(root->get<"bar">().get<"value">() == 75);
}

void c_code_shares_the_subject() {
  lv_observer_t* o = lv_subject_add_observer(speed.raw(), &c_observer, nullptr);
  speed = 33;
  CHECK(c_side_value == 33);
  lv_subject_set_int(speed.raw(), 44);  // C writes, C++ reads
  CHECK(speed.get() == 44);
  lv_observer_delete(o);
}

void observers() {
  static int seen = -1;
  lv_observer_t* a = speed.observe([](int v) { seen = v; });
  CHECK(seen == speed.get());  // called once right away
  speed = 9;
  CHECK(seen == 9);

  int captured = -1;
  lv_observer_t* b = speed.observe([&captured](int v) { captured = v; });
  speed = 10;
  CHECK(captured == 10);
  lv_observer_delete(a);
  lv_observer_delete(b);
  speed = 11;
  CHECK(seen == 10 && captured == 10);
}

void runtime_binding(lv::obj screen) {
  auto l = screen.add<"label">().set({.text = lv::bind(speed, "%d")});
  speed = 5;
  CHECK(std::strcmp(l.get<"text">(), "5") == 0);

  // Deleting the widget removes its observer: setting the state must be safe.
  lv_obj_delete(l.raw());
  speed = 6;
  CHECK(speed.get() == 6);
}

void survives_a_deleted_tree(lv::obj screen) {
  { auto root = lv::mount<ui>(screen); }
  speed = 77;
  hidden = true;
  CHECK(speed.get() == 77 && hidden.get());
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
