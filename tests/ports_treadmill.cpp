// The treadmill panel (examples/ports/qt/treadmill): every scene is shown and compared with a
// reference picture, and the buttons and the clocks do what the original's do.
//
//   ports_treadmill <reference prefix>     e.g. tests/ref/treadmill_ for treadmill_light.png, ...
//
// A picture is compared as a file: the screen is written with lv::write_png, which is the same
// bytes for the same pixels, and the reference is what treadmill_shot wrote. LVGL's own screenshot
// compare decodes the reference into the heap, and 800 x 480 pixels do not fit in the 4 MB heap
// of the tests.

#include "check.hpp"

#include <lv/lv.hpp>
#include <scenes.hpp>
#include <shell.hpp>
#include <state.hpp>

#include <lv/png.hpp>

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

namespace {

using namespace treadmill;
using mp_units::si::unit_symbols::s;

constinit lv::frame<screen_width, screen_height> frame;

std::string contents_of(const std::string& path) {
  std::ifstream in{path, std::ios::binary};
  return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

// Whether the screen shows what the reference file shows.
bool matches(lv::obj screen, const std::string& reference) {
  const std::string taken = "ports_treadmill_screen.png";
  const std::string expected = contents_of(reference);
  return lv::write_png(screen, taken.c_str()) && !expected.empty() && contents_of(taken) == expected;
}

int tenths() { return speed.get().numerical_value_in(tenth_mph); }
int percent() { return incline.get().value; }

void run_for(lv::application& app, int ms) {
  for (int at = 0; at < ms; at += step_ms) {
    app.advance(step_ms * lv::ms);
    app.step();
  }
}

// The arrows move a value by one, and stop at its ends.
void the_arrows_stop_at_the_ends() {
  stop();
  slower();
  flatter();
  check(tenths() == 0 && percent() == 0);
  faster();
  steeper();
  check(tenths() == 1 && percent() == 1);
  for (int press = 0; press < 300; ++press) faster();
  for (int press = 0; press < 300; ++press) steeper();
  check(tenths() == 200 && percent() == 100);
  stop();
  check(tenths() == 0 && percent() == 0);
}

// Start counts a second at a time. The pace is shown as minutes and seconds for a mile.
void a_run_counts_and_stop_clears(lv::application& app) {
  stop();
  for (int press = 0; press < 100; ++press) faster();  // 10.0 mph: a mile in 6:00
  start();
  run_for(app, 3100);
  check(std::strcmp(time_text.get(), "00:03") == 0);
  check(std::strcmp(speed_text.get(), "10.0") == 0);
  check(std::strcmp(pace_text.get(), "6:00") == 0);
  check(heart_rate.get().value >= 100 && heart_rate.get().value < 200);
  check(runner_moving.get() && runner_frame.get().value > 0);
  stop();
  check(std::strcmp(time_text.get(), "00:00") == 0 && std::strcmp(pace_text.get(), "0:00") == 0);
  check(std::strcmp(speed_text.get(), "0.0") == 0 && heart_rate.get().value == 0);
  check(!runner_moving.get() && runner_frame.get().value == 0);
}

// At a standstill a mile takes for ever, and the original shows 99:60.
void a_standstill_shows_the_longest_pace(lv::application& app) {
  stop();
  start();
  run_for(app, 1100);
  check(std::strcmp(pace_text.get(), "99:60") == 0);
  stop();
}

// Pause stops the counting and the dot, and a second Pause goes on from where they were.
void pause_goes_on_where_it_stopped(lv::application& app) {
  stop();
  faster();
  start();
  run_for(app, 2100);
  pause();
  const int frame_then = runner_frame.get().value;
  run_for(app, 2000);
  check(paused.get() && !runner_moving.get() && runner_frame.get().value == frame_then);
  check(std::strcmp(time_text.get(), "00:02") == 0);
  pause();
  run_for(app, 1100);
  check(!paused.get() && runner_frame.get().value > frame_then);
  check(std::strcmp(time_text.get(), "00:03") == 0);
  stop();
}

// Auto mode raises the incline and the speed each second, and Start takes it back to manual.
void auto_mode_walks_the_slope(lv::application& app) {
  stop();
  toggle_mode();
  run_for(app, 3100);
  // Each second sets the height again from the incline: 3 percent is 0 feet.
  check(auto_mode.get() && percent() == 3 && tenths() == 9 && elevation.get().value == 0);
  start();
  check(!auto_mode.get());
  toggle_mode();
  toggle_mode();
  check(!runner_moving.get());
  stop();
}

// The dot is where the original's keyframes put it.
void the_dot_follows_the_keyframes() {
  check(runner_at(0).x == 44 && runner_at(0).y == 119);
  check(runner_at(100).x == 101 && runner_at(100).y == 119);
  check(runner_at(340).x == 211 && runner_at(340).y == 171);
  check(runner_at(600).x == 101 && runner_at(600).y == 231);
  check(runner_at(1000).x == 44 && runner_at(1000).y == 119);
}

// A look is a state; the screen is built again for it, and keeps the readings.
void a_look_keeps_the_readings(lv::application& app) {
  stop();
  faster();
  choose_look<look::dark>();
  run_for(app, 50);
  check(active_look.get() == look::dark && tenths() == 1);
  choose_look<look::light>();
  run_for(app, 50);
  check(active_look.get() == look::light);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: ports_treadmill <reference prefix>\n");
    return 2;
  }
  lv::application app;
  lv::display display{app, frame};
  check(treadmill::start(display.screen()));
  the_dot_follows_the_keyframes();
  the_arrows_stop_at_the_ends();
  for (const treadmill::scene& scene : treadmill::scenes) {
    treadmill::play(app, scene);
    const std::string reference = std::string{argv[1]} + scene.name + ".png";
    check(matches(display.screen(), reference));
  }
  stop();
  a_run_counts_and_stop_clears(app);
  a_standstill_shows_the_longest_pace(app);
  pause_goes_on_where_it_stopped(app);
  auto_mode_walks_the_slope(app);
  a_look_keeps_the_readings(app);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
