// lv::pointer: a mouse or a touch panel fed by the platform through a queue in the object,
// or asked by LVGL at each read. Every sample reaches LVGL, also a press shorter than LVGL's
// read period, and LVGL reads at the next step.
#include <lv/lv.hpp>
#include <lv/pointer.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace {

using namespace lv::units;

int clicks = 0;
void clicked() { ++clicks; }

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::button({.id = "go", .on_clicked = &clicked, .size = {100 * px, 50 * px}})[lv::label({.text = "Go"})],
    lv::slider({.id = "level", .range = {0, 100}, .size = {200 * px, 20 * px}})];

constinit lv::frame<320, 240> frame;

// The deduced pointers: a queue of 16 by default, or none for a polled one.
static_assert(std::is_same_v<decltype(lv::pointer{std::declval<lv::display&>()}), lv::pointer<16>>);
lv::pointer_sample asked();
static_assert(std::is_same_v<decltype(lv::pointer{std::declval<lv::display&>(), &asked}), lv::pointer<0>>);
static_assert(std::is_same_v<decltype(lv::pointer{std::declval<lv::display&>(), [] { return lv::pointer_sample{}; }}),
                             lv::pointer<0>>);
static_assert(!std::is_copy_constructible_v<lv::pointer<>> && !std::is_move_constructible_v<lv::pointer<>>);

// The middle of a widget, where a finger would press it.
lv::point middle(lv::obj widget) {
  lv_area_t at;
  lv_obj_get_coords(widget.raw(), &at);
  return {(at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2};
}

// The left and right ends of a widget, in its middle line.
std::pair<lv::point, lv::point> ends(lv::obj widget) {
  lv_area_t at;
  lv_obj_get_coords(widget.raw(), &at);
  const std::int32_t y = (at.y1 + at.y2) / 2;
  return {{at.x1 + 2, y}, {at.x2 - 2, y}};
}

// LVGL keeps the device and its read timer on its heap; lvgl26 nothing.
void costs_two_blocks(lv::application& app, lv::display& display) {
  const auto before = app.heap();
  {
    lv::pointer mouse{display};
    const auto with = app.heap();
    check(mouse.raw() != nullptr && with.used > before.used);
    std::printf("a pointer: %zu bytes of LVGL's heap\n", with.used - before.used);
  }
  check(app.heap().used == before.used);
}

// A press and a release before LVGL reads are one click, read at the next step with no time
// passed. Two clicks are two.
void a_short_press_is_a_click(lv::application& app, lv::display& display, lv::view<"button"> go) {
  lv::pointer mouse{display};
  clicks = 0;
  check(mouse.press(middle(go)) && mouse.release());
  app.step();
  check(clicks == 1);
  check(mouse.press(middle(go)) && mouse.release() && mouse.press(middle(go)) && mouse.release());
  app.step();
  check(clicks == 3);
}

// A press, two moves and a release, all before LVGL reads, drag the slider to its end.
void a_drag_moves_a_slider(lv::application& app, lv::display& display, lv::view<"slider"> level) {
  lv::pointer mouse{display};
  const auto [left, right] = ends(level);
  check(mouse.press(left) && mouse.move({(left.x + right.x) / 2, left.y}) && mouse.move(right) && mouse.release());
  app.step();
  check(level.get<"value">() > 90);
}

// A full queue merges a move into its newest sample and refuses a press or a release.
void a_full_queue(lv::application& app, lv::display& display, lv::view<"slider"> level) {
  lv::pointer<2> mouse{display};
  const auto [left, right] = ends(level);
  lv_slider_set_value(level.raw(), 0, LV_ANIM_OFF);
  check(mouse.press(left) && mouse.move({left.x + 10, left.y}));
  check(mouse.move(right));            // merged into the move before
  check(!mouse.release());             // no room, and nothing to merge it into
  app.step();
  check(level.get<"value">() > 90);    // the merged move reached LVGL
  check(mouse.release());
  app.step();
}

// A polled pointer: LVGL asks the function at each read of its period.
lv::pointer_sample now{};
lv::pointer_sample asked() { return now; }

// One press and release of `now`, each held for one read period of LVGL.
void press_and_release(lv::application& app, lv::view<"button"> go) {
  now = {middle(go), lv::pressed};
  app.advance(40 * ms), app.step();
  now.state = lv::released;
  app.advance(40 * ms), app.step();
}

void a_polled_pointer(lv::application& app, lv::display& display, lv::view<"button"> go) {
  clicks = 0;
  {
    lv::pointer panel{display, &asked};
    press_and_release(app, go);
  }
  {
    lv::pointer panel{display, [] { return now; }};  // a lambda without captures
    press_and_release(app, go);
  }
  check(clicks == 2);
}

// A call that breaks a contract ends the program with a contract violation.
int violates(const char* which, lv::display& display) {
  if (std::strcmp(which, "no_function") == 0) {
    lv::pointer_sample (*none)() = nullptr;
    lv::pointer panel{display, none};
    return panel.raw() != nullptr ? 1 : 0;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  lv::application app;
  lv::display display{app, frame};
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1], display));
    return 0;
  }
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  app.advance(40 * ms), app.step();  // the layout: the widgets get their places

  costs_two_blocks(app, display);
  a_short_press_is_a_click(app, display, root->get<"go">());
  a_drag_moves_a_slider(app, display, root->get<"level">());
  a_full_queue(app, display, root->get<"level">());
  a_polled_pointer(app, display, root->get<"go">());

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
