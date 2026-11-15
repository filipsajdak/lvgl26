// lv::application owns LVGL's lifetime and its loop. lv::display owns a display
// and tells a panel what was drawn.
//
//   application          runs the checks
//   application twice    starts a second application: must end with a contract violation

#include <lv/host.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

namespace {

using namespace lv::units;

static_assert(!std::is_copy_constructible_v<lv::application>);
static_assert(!std::is_copy_constructible_v<lv::display>);
static_assert(lv::frame<64, 48>::width == 64 && lv::frame<64, 48>::height == 48);
static_assert(lv::band<64, 48, 8>::lines == 8);
static_assert(sizeof(lv::band<64, 48, 8>) < sizeof(lv::frame<64, 48>));

constexpr auto ui = lv::column({.size = {64 * px, 48 * px}})[lv::label({.id = "title", .text = "Hi"})];

// A clock and a wait that the test controls.
constinit std::uint32_t now_ms = 0;
constinit int idled = 0;
constinit int idled_at_quit = -1;

lv::duration test_clock() { return {now_ms}; }

void test_idle(lv::duration) {
  ++idled;
  now_ms += 5;
}

lv::task<> stop_after(lv::application& app, lv::duration wait, int result) {
  co_await lv::sleep(wait);
  idled_at_quit = idled;
  app.quit(result);
}

void starts_and_stops() {
  check(!lv_is_initialized());
  for (int i = 0; i < 2; ++i) {  // LVGL can start again after it stopped
    lv::application app;
    check(lv_is_initialized());
  }
  check(!lv_is_initialized());
}

void time_is_told() {
  lv::application app;
  const std::uint32_t before = lv_tick_get();
  app.advance(7 * ms);
  check(lv_tick_get() - before == 7);
}

void time_comes_from_a_clock() {
  now_ms = 100;
  lv::application app{{.clock = &test_clock}};
  check(lv_tick_get() == 100);
  now_ms = 250;
  check(lv_tick_get() == 250);
}

void runs_until_quit() {
  now_ms = 0;
  idled = 0;
  lv::application app{{.clock = &test_clock, .idle = &test_idle}};

  lv::spawn(stop_after(app, 50 * ms, 7));
  check(app.run() == 7);
  check(now_ms >= 50);            // the time passed in the waits between the steps
  check(idled > 0);
  check(idled == idled_at_quit);  // no wait after quit()

  lv::spawn(stop_after(app, 20 * ms, 3));  // run() can be called again
  check(app.run() == 3);
}

void runs_on_the_hosts_clock() {
  lv::application app{lv::host};
  const std::uint32_t before = lv_tick_get();
  lv::spawn(stop_after(app, 30 * ms, 5));
  check(app.run() == 5);
  check(lv_tick_get() - before >= 30);
}

bool one_value(std::span<const std::byte> bytes) {
  for (std::byte b : bytes)
    if (b != bytes[0]) return false;
  return true;
}

// Draws what is due: the screen is refreshed about every 33 ms.
void refresh(lv::application& app) {
  app.advance(50 * ms);
  app.step();
}

constinit lv::frame<64, 48> frame;
constinit int changes = 0;
constinit long changed_pixels = 0;

void changed(lv::area where) {
  ++changes;
  changed_pixels += static_cast<long>(where.width) * where.height;
}

struct change_log {
  lv::area last;
  int calls = 0;
  void operator()(lv::area where) {
    last = where;
    ++calls;
  }
};

void a_frame_holds_the_picture() {
  lv::application app;
  {
    lv::display display{app, frame};
    check(display.width() == 64 && display.height() == 48);
    check(display.screen().raw() == lv_screen_active());
    check(display.raw() == lv_display_get_default());
    auto root = lv::mount<ui>(display.screen());
    check(root.has_value());
    check(one_value(frame.bytes()));  // nothing is drawn yet
    refresh(app);
    check(!one_value(frame.bytes()));  // the text on its background
  }
  check(lv_display_get_default() == nullptr);  // the display ended with its object
}

void a_frame_names_what_changed() {
  lv::application app;
  changes = 0;
  changed_pixels = 0;
  {
    lv::display display{app, frame, &changed};
    auto root = lv::mount<ui>(display.screen());
    refresh(app);
    check(changes >= 1);
    check(changed_pixels >= 64L * 48);  // the first drawing covers the screen

    const int before = changes;
    root->get<"title">().set({.text = "Ho"});
    refresh(app);
    check(changes > before);  // only when something changed
    const int after = changes;
    refresh(app);
    check(changes == after);
  }
  {
    change_log log;
    lv::display display{app, frame, log};  // a named object instead of a function
    auto root = lv::mount<ui>(display.screen());
    refresh(app);
    check(log.calls >= 1);
    check(log.last == lv::area{0, 0, 64, 48});
  }
}

constinit lv::band<64, 48, 8> band;

struct pieces {
  long pixels = 0;
  std::size_t bytes = 0;
  int calls = 0;
  int tallest = 0;
  bool inside = true;
  void operator()(lv::area where, std::span<const std::byte> piece) {
    ++calls;
    pixels += static_cast<long>(where.width) * where.height;
    bytes += piece.size();
    tallest = where.height > tallest ? where.height : tallest;
    inside = inside && where.x >= 0 && where.y >= 0 && where.x + where.width <= 64 && where.y + where.height <= 48;
  }
};

constinit pieces sent_to_function{};
void send(lv::area where, std::span<const std::byte> piece) { sent_to_function(where, piece); }

void expect_whole_screen(const pieces& got) {
  const std::size_t pixel = lv_color_format_get_size(LV_COLOR_FORMAT_NATIVE);
  check(got.calls >= 6);  // 48 lines, at most 8 at a time
  check(got.tallest <= 8);
  check(got.inside);
  check(got.pixels == 64L * 48);
  check(got.bytes == 64u * 48u * pixel);
}

void a_band_hands_over_pieces() {
  lv::application app;
  {
    pieces panel;
    lv::display display{app, band, panel};
    check(display.width() == 64 && display.height() == 48);
    auto root = lv::mount<ui>(display.screen());
    refresh(app);
    expect_whole_screen(panel);
  }
  {
    sent_to_function = {};
    lv::display display{app, band, &send};
    auto root = lv::mount<ui>(display.screen());
    refresh(app);
    expect_whole_screen(sent_to_function);
  }
}

void the_heap_is_measured() {
  lv::application app;
  const lv::heap_use before = app.heap();
  check(before.total > 0 && before.used > 0 && before.used < before.total);
  check(before.largest_free <= before.total - before.used);

  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  const lv::heap_use after = app.heap();
  check(after.used > before.used);
  check(after.used < after.total);
}

int violates(const char* name) {
  if (std::strcmp(name, "twice") == 0) {
    lv::application first;
    lv::application second;
    return 1;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1]));
    return 0;
  }
  starts_and_stops();
  time_is_told();
  time_comes_from_a_clock();
  runs_until_quit();
  runs_on_the_hosts_clock();
  a_frame_holds_the_picture();
  a_frame_names_what_changed();
  a_band_hands_over_pieces();
  the_heap_is_measured();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
