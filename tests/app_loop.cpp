// The application's loop and what joins it: event sources polled before each step, one of
// them waiting between steps, a window that feeds a pointer and ends run() when it is
// closed, and a display that draws nothing while it is hidden. A fake window stands in for
// SDL, so the test runs everywhere.
#include <lv/lv.hpp>
#include <lv/pointer.hpp>

#include "check.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

namespace {

using namespace lv::units;

constinit lv::frame<160, 120> frame;

enum class close_action { quit, ignore, hide };

// A window as a platform gives one: it joins the loop, is the panel of the display, feeds a
// pointer, and can be closed, hidden and shown. Its "events" are what the test queues.
struct fake_window {
  struct event {
    enum kind { press, release, close } what;
    lv::point at;
  };

  lv::application& app;
  std::vector<event> events;
  lv::display* display = nullptr;
  lv::pointer_feed mouse{};
  close_action on_close = close_action::quit;
  bool hidden = false;
  int polls = 0;
  int waits = 0;
  int drawn = 0;
  lv::in_loop<fake_window> loop;

  explicit fake_window(lv::application& a) : app(a), loop(a, *this) {}

  void poll() {
    ++polls;
    for (const event& e : events) handle(e);
    events.clear();
  }
  void wait(lv::duration) { ++waits; }

  void handle(const event& e) {
    if (e.what == event::close) return closed();
    static_cast<void>(mouse(e.at, e.what == event::press ? lv::pressed : lv::released));
  }
  void closed() {
    if (on_close == close_action::quit) app.quit();
    if (on_close == close_action::hide) hide();
  }

  void operator()(lv::area) { ++drawn; }
  void attach(lv::display& d) { display = &d; }
  void detach(lv::display&) { display = nullptr; }
  void attach(lv::pointer_feed f) { mouse = f; }
  void detach(lv::pointer_feed f) {
    if (mouse == f) mouse = {};
  }
  void hide() {
    hidden = true;
    display->set_visible(false);
  }
  void show() {
    hidden = false;
    display->set_visible(true);
  }
};

// A source that only polls, and one that ends another source while it is polled.
struct counter_source {
  int polls = 0;
  lv::in_loop<counter_source> loop;
  explicit counter_source(lv::application& a) : loop(a, *this) {}
  void poll() { ++polls; }
};

struct ender {
  std::optional<counter_source>* victim;
  lv::in_loop<ender> loop;
  ender(lv::application& a, std::optional<counter_source>& v) : victim(&v), loop(a, *this) {}
  void poll() { victim->reset(); }
};

static_assert(lv::loop_source<fake_window> && lv::waiting_loop_source<fake_window>);
static_assert(lv::loop_source<counter_source> && !lv::waiting_loop_source<counter_source>);
static_assert(lv::pointer_feeder<fake_window> && lv::attached_panel<fake_window> && lv::hideable_panel<fake_window>);
static_assert(std::is_same_v<decltype(lv::pointer{std::declval<lv::display&>(), std::declval<fake_window&>()}), lv::pointer<16>>);

int clicks = 0;
void clicked() { ++clicks; }

constexpr auto ui = lv::column({.size = {160 * px, 120 * px}})[
    lv::button({.id = "go", .on_clicked = &clicked, .size = {80 * px, 40 * px}})[lv::label({.id = "text", .text = "Go"})]];

lv::point middle(lv::obj widget) {
  lv_area_t at;
  lv_obj_get_coords(widget.raw(), &at);
  return {(at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2};
}

std::size_t heap_used(lv::application& app) { return app.heap().used; }

// run() polls the window, which feeds its pointer, and ends when the window is closed.
void run_until_closed(lv::application& app, fake_window& window, lv::obj button) {
  clicks = 0;
  const lv::point at = middle(button);
  window.events = {{fake_window::event::press, at}, {fake_window::event::release, at}, {fake_window::event::close, at}};
  const int polls = window.polls;
  check(app.run() == 0);
  check(clicks == 1 && window.polls == polls + 1 && window.waits == 0);  // closed in the first poll
}

// A close request that the program takes instead: ignored, run() goes on; hidden, run()
// goes on and the display draws nothing.
void close_taken(lv::application& app, fake_window& window, lv::display& display) {
  window.on_close = close_action::ignore;
  window.events = {{fake_window::event::close, {}}};
  app.step();
  check(!window.hidden && display.visible());
  window.on_close = close_action::hide;
  window.events = {{fake_window::event::close, {}}};
  app.step();
  check(window.hidden && !display.visible());
  window.show();
  window.on_close = close_action::quit;
}

// A hidden display draws nothing; shown again, it draws its whole screen once.
void hidden_draws_nothing(lv::application& app, fake_window& window, lv::display& display, lv::view<"label"> text) {
  display.hide();  // the panel hides itself
  check(window.hidden && !display.visible());
  std::array<std::byte, sizeof(frame.memory)> before{};
  std::memcpy(before.data(), frame.memory, before.size());
  const int drawn = window.drawn;
  text.set({.text = "Changed"});
  for (int i = 0; i < 5; ++i) app.advance(40 * ms), app.step();
  check(window.drawn == drawn && std::memcmp(before.data(), frame.memory, before.size()) == 0);
  display.show();
  app.advance(40 * ms), app.step();
  check(window.drawn > drawn && std::memcmp(before.data(), frame.memory, before.size()) != 0);
}

// The visible state is told to what follows it, without an allocation.
int told = 0;
void count_told(lv::detail::watch_node&) { ++told; }

void visible_is_observable(lv::application& app, lv::display& display) {
  lv::detail::watch_node node;
  node.changed = &count_told;
  const std::size_t heap = heap_used(app);
  lv::detail::display_access::visible_watchers(display).append(node);
  display.set_visible(false);
  display.set_visible(false);  // no change, nobody is told
  display.set_visible(true);
  check(told == 2 && heap_used(app) == heap);
  lv::detail::watch_list::remove(node);
}

void nothing_drawn(lv::area) {}

// A display whose panel is a function has no window to hide: the request changes only its
// own state.
void a_function_panel_only_stops_drawing(lv::application& app) {
  static constinit lv::frame<32, 32> small;
  lv::display plain{app, small, &nothing_drawn};
  plain.hide();
  check(!plain.visible());
  plain.show();
  check(plain.visible());
}

// display_of finds the lvgl26 display a widget is on.
void display_of_a_widget(lv::display& display, lv::obj button) { check(&lv::display_of(button) == &display); }

// The first source that can wait waits; without one, the application's idle function does.
int idled = 0;
void idle(lv::duration) { ++idled; }

void the_wait(lv::application& app, fake_window& window) {
  std::optional<counter_source> other{std::in_place, app};
  ender stop{app, other};  // joined after `other`: polled after it, and ends it
  window.events = {{fake_window::event::close, {}}};
  const int waits = window.waits;
  check(app.run() == 0 && window.waits == waits && !other.has_value());
}

// A source that ends with its application leaves the loop and later ends without it.
void a_source_outlives_its_application() {
  std::optional<counter_source> survivor;
  {
    lv::application app;
    const std::size_t heap = app.heap().used;
    survivor.emplace(app);
    check(survivor->loop.joined() && app.heap().used == heap);  // joining allocates nothing
    app.step();
    check(survivor->polls == 1);
  }
  check(!survivor->loop.joined());
  survivor.reset();
}

// A task that belongs to a screen ends with it; mounting again runs exactly one.
constinit lv::state ticks{lv::counter{}};
lv::task<> tick() {
  for (;;) {
    co_await lv::sleep(100 * ms);
    ++ticks;
  }
}

void a_task_tied_to_its_root(lv::application& app, lv::display& display) {
  constexpr std::int32_t steps = 10;
  {
    auto root = lv::mount<ui>(display.screen());
    check(lv::spawn(tick(), root.root()));
    for (int i = 0; i < steps; ++i) app.advance(50 * ms), app.step();
    check(ticks.get().value > 0);
  }
  const std::int32_t after_unmount = ticks.get().value;
  for (int i = 0; i < steps; ++i) app.advance(50 * ms), app.step();
  check(ticks.get().value == after_unmount);  // the task ended with the tree
  auto root = lv::mount<ui>(display.screen());
  check(lv::spawn(tick(), root.root()));
  const std::int32_t start = ticks.get().value;
  for (int i = 0; i < steps; ++i) app.advance(50 * ms), app.step();  // 500 ms: five ticks of one task
  check(ticks.get().value - start == 5);
}

// A widget on a display made through the C API has no lv::display: display_of breaks its
// precondition. Run as a test of its own, which expects the violation.
int display_of_a_c_display() {
  lv::application app;
  lv_display_t* raw = lv_display_create(32, 32);
  lv_obj_t* label = lv_label_create(lv_display_get_screen_active(raw));
  return lv::display_of(lv::obj{label}).width();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::strcmp(argv[1], "display_of_c_display") == 0) {
    std::printf("no contract violation: %d\n", display_of_a_c_display());
    return 0;
  }
  a_source_outlives_its_application();
  lv::application app{{.idle = &idle}};
  fake_window window{app};
  lv::display display{app, frame, window};
  lv::pointer mouse{display, window};
  check(window.display == &display && static_cast<bool>(window.mouse));
  {
    auto root = lv::mount<ui>(display.screen());
    if (!root) return 1;
    app.advance(40 * ms), app.step();
    run_until_closed(app, window, root->get<"go">());
    close_taken(app, window, display);
    hidden_draws_nothing(app, window, display, root->get<"text">());
    display_of_a_widget(display, root->get<"go">());
  }
  visible_is_observable(app, display);
  a_function_panel_only_stops_drawing(app);
  the_wait(app, window);
  a_task_tied_to_its_root(app, display);
  check(idled == 0);  // the window waits, so the idle function never runs
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
