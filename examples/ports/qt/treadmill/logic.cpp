// What the panel does: the numbers move as they do in the original, and the dot goes round
// the track. The buttons and the two clocks are here; the pages know nothing of the rules.

#include "state.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <span>

namespace treadmill {

// The states the pages bind to. They are defined here and declared in state.hpp: the address of an
// inline variable is not a constant under a sanitizer, and a binding needs it as one.
constinit lv::state<look> active_look{look::light};
constinit lv::state<lv::counter> heart_rate{lv::counter{0}};
constinit lv::state<lv::counter> elevation{lv::counter{0}};
constinit lv::state<lv::counter> incline{lv::counter{0}};
constinit lv::state<speed_t> speed{speed_t{}};
constinit lv::state<seconds_t> elapsed{seconds_t{}};
constinit lv::state<seconds_t> pace{seconds_t{}};
constinit lv::state<bool> clock_running{false};
constinit lv::state<bool> auto_mode{false};
constinit lv::state<bool> paused{false};
constinit lv::state<bool> runner_moving{false};
constinit lv::state<lv::counter> runner_frame{lv::counter{0}};
constinit lv::state<const char*> time_text{"00:00"};
constinit lv::state<const char*> pace_text{"0:00"};
constinit lv::state<const char*> speed_text{"0.0"};
constinit manual_mode_t manual_mode{&is_manual, auto_mode};

namespace {

using namespace lv::units;
using mp_units::si::unit_symbols::s;

// ---- the numbers, as the original moves them -----------------------------------------

constexpr int bpm_lowest            = 100;  // the heart rate is a number from 100 to 199
constexpr int bpm_spread            = 100;
constexpr int percent_most          = 100;
constexpr int percent_least         = 0;
constexpr int speed_most            = 200;  // tenths: 20.0 mph
constexpr int speed_step            = 1;    // tenths a button press moves it
constexpr int auto_speed_step       = 3;
constexpr int auto_incline_top      = 99;  // auto mode turns round above this incline
constexpr int auto_incline_bottom   = 1;
constexpr int feet_per_percent      = 33;
constexpr int seconds_per_minute    = 60;
constexpr int tenths                = 10;
constexpr int pace_minutes_most     = 99;  // a pace above it is shown as 99:60
constexpr int pace_clamped          = pace_minutes_most * seconds_per_minute + seconds_per_minute;
constexpr double incline_pace_share = 10.0;  // an incline of 10 percent adds one minute to a mile
constexpr double tenths_per_hour_minutes = 600.0;  // minutes for a mile at a speed of one tenth

// The original draws its heart rate at random. A generator with a fixed start gives the same
// readings in every run, so the test's pictures stay the same.
constexpr std::uint32_t random_start      = 12345;
constexpr std::uint32_t random_multiplier = 1664525;
constexpr std::uint32_t random_increment  = 1013904223;
constexpr int random_shift                = 16;  // the high bits are the better ones

std::uint32_t random_state = random_start;

int next_random(int below) {
  random_state = random_state * random_multiplier + random_increment;
  return static_cast<int>((random_state >> random_shift) % static_cast<std::uint32_t>(below));
}

bool going_up = true;  // auto mode: the incline rises until it is nearly at its top

int count_of(const lv::state<lv::counter>& c) { return c.get().value; }
int tenths_of(const lv::state<speed_t>& v) { return v.get().numerical_value_in(tenth_mph); }

// How long a mile takes at this speed and incline.
seconds_t pace_for(int speed_in_tenths, int incline_in_percent) {
  const double minutes = speed_in_tenths > 0 ? tenths_per_hour_minutes / speed_in_tenths +
                                                 incline_in_percent / incline_pace_share
                                             : static_cast<double>(pace_minutes_most + 1);
  if (minutes > pace_minutes_most) return pace_clamped * s;
  const int whole = static_cast<int>(minutes);
  const int part  = static_cast<int>((minutes - whole) * seconds_per_minute);
  return (whole * seconds_per_minute + part) * s;
}

// One second of counting: a new heart rate, a second more, and the pace and the height that follow.
void calculate() {
  heart_rate = lv::counter{bpm_lowest + next_random(bpm_spread)};
  elapsed    = elapsed.get() + 1 * s;
  pace       = pace_for(tenths_of(speed), count_of(incline));
  elevation  = lv::counter{count_of(incline) * feet_per_percent / percent_most};
}

// The dot starts its lap, from the beginning unless the machine was paused.
void begin_track() {
  if (runner_moving.get()) return;
  if (!paused.get()) runner_frame = lv::counter{0};
  runner_moving = true;
}

// One second of auto mode: the incline and the speed walk up to the top and back.
void automate() {
  begin_track();
  const int direction = going_up ? 1 : -1;
  elevation           = lv::counter{count_of(elevation) + direction};
  incline             = lv::counter{count_of(incline) + direction};
  speed               = speed.get() + direction * speed_t{auto_speed_step * tenth_mph};
  if (going_up && count_of(incline) > auto_incline_top)
    going_up = false;
  else if (!going_up && count_of(incline) < auto_incline_bottom)
    going_up = true;
  calculate();
}

// ---- the text of the readings --------------------------------------------------------

constexpr std::size_t text_size = 16;  // room for any int, so the compiler sees no truncation
char time_buffer[text_size];
char pace_buffer[text_size];
char speed_buffer[text_size];

void show_time(seconds_t t) {
  const int total = t.numerical_value_in(s);
  std::snprintf(
    time_buffer,
    sizeof time_buffer,
    "%02d:%02d",
    total / seconds_per_minute,
    total % seconds_per_minute
  );
  time_text = time_buffer;
}

void show_pace(seconds_t t) {
  const int total = t.numerical_value_in(s);
  if (total >= pace_clamped)
    std::snprintf(pace_buffer, sizeof pace_buffer, "%d:%d", pace_minutes_most, seconds_per_minute);
  else
    std::snprintf(
      pace_buffer,
      sizeof pace_buffer,
      "%d:%02d",
      total / seconds_per_minute,
      total % seconds_per_minute
    );
  pace_text = pace_buffer;
}

void show_speed(speed_t v) {
  const int whole  = v.numerical_value_in(tenth_mph);
  const char* sign = whole < 0 ? "-" : "";
  std::snprintf(
    speed_buffer,
    sizeof speed_buffer,
    "%s%d.%d",
    sign,
    std::abs(whole) / tenths,
    std::abs(whole) % tenths
  );
  speed_text = speed_buffer;
}

constinit lv::effect time_follows{&show_time, elapsed};
constinit lv::effect pace_follows{&show_pace, pace};
constinit lv::effect speed_follows{&show_speed, speed};

// ---- the track's timeline ------------------------------------------------------------

struct key {
  int frame;
  int value;
};

// The keyframes of the original's timeline, in order of their frames.
constexpr std::array across{key{0, 44},    key{200, 158}, key{220, 168}, key{240, 178},
                            key{260, 188}, key{280, 198}, key{300, 204}, key{320, 210},
                            key{340, 211}, key{380, 210}, key{400, 204}, key{420, 198},
                            key{440, 188}, key{460, 178}, key{480, 168}, key{500, 158},
                            key{700, 44},  key{720, 34},  key{740, 24},  key{760, 14},
                            key{780, 4},   key{800, -2},  key{820, -6},  key{840, -9},
                            key{860, -9},  key{880, -6},  key{900, -2},  key{920, 4},
                            key{940, 14},  key{960, 24},  key{980, 34},  key{1000, 44}};
constexpr std::array down{key{0, 119},   key{200, 119}, key{220, 120}, key{240, 123}, key{260, 128},
                          key{280, 136}, key{300, 146}, key{320, 157}, key{340, 171}, key{360, 185},
                          key{380, 196}, key{400, 207}, key{420, 215}, key{440, 221}, key{460, 226},
                          key{480, 230}, key{500, 231}, key{720, 230}, key{740, 226}, key{760, 221},
                          key{780, 215}, key{800, 207}, key{820, 196}, key{840, 185}, key{860, 171},
                          key{880, 157}, key{900, 146}, key{920, 136}, key{940, 128}, key{960, 123},
                          key{980, 120}, key{1000, 119}};

// A value between two keyframes, on a straight line.
int between(std::span<const key> keys, int frame) {
  const auto after = std::ranges::find_if(keys, [frame](const key& k) {
    return k.frame >= frame;
  });
  if (after == keys.begin()) return after->value;
  if (after == keys.end()) return keys.back().value;
  const key& before = *(after - 1);
  return before.value +
         (after->value - before.value) * (frame - before.frame) / (after->frame - before.frame);
}

constexpr int tick_ms         = 20;  // the dot is placed every 20 ms
constexpr int frames_per_tick = 2;   // a lap is 1000 frames in 10 s

}  // namespace

spot runner_at(int frame) { return {between(across, frame), between(down, frame)}; }

// ---- what the buttons do -------------------------------------------------------------

void start() {
  clock_running = true;
  begin_track();
  auto_mode = false;
  paused    = false;
}

void pause() {
  clock_running = false;
  auto_mode     = false;
  if (!paused.get()) {
    runner_moving = false;
    paused        = true;
    return;
  }
  clock_running = true;
  begin_track();
  paused = false;
}

void stop() {
  clock_running = false;
  runner_moving = false;
  runner_frame  = lv::counter{0};
  auto_mode     = false;
  heart_rate    = lv::counter{0};
  elevation     = lv::counter{0};
  incline       = lv::counter{0};
  elapsed       = 0 * s;
  pace          = 0 * s;
  speed         = speed_t{};
  paused        = false;
}

void toggle_mode() {
  auto_mode = !auto_mode.get();
  if (!auto_mode.get()) runner_moving = false;
}

void flatter() {
  if (count_of(incline) > percent_least) incline = lv::counter{count_of(incline) - 1};
}

void steeper() {
  if (count_of(incline) < percent_most) incline = lv::counter{count_of(incline) + 1};
}

void slower() {
  if (tenths_of(speed) > 0) speed = speed.get() - speed_t{speed_step * tenth_mph};
}

void faster() {
  if (tenths_of(speed) < speed_most) speed = speed.get() + speed_t{speed_step * tenth_mph};
}

bool start_readouts() {
  return time_follows.start() && pace_follows.start() && speed_follows.start();
}

lv::task<> run_clock() {
  for (;;) {
    co_await lv::sleep(1000 * ms);
    if (clock_running.get()) calculate();
  }
}

lv::task<> run_auto() {
  for (;;) {
    co_await lv::sleep(1000 * ms);
    if (auto_mode.get()) automate();
  }
}

lv::task<> follow_runner(lv::view<"image"> runner) {
  for (;;) {
    const spot at = runner_at(runner_frame.get().value);
    runner.set({.x = (track_view.x + at.x) * px, .y = (track_view.y + at.y) * px});
    co_await lv::sleep(tick_ms * ms);
    if (runner_moving.get())
      runner_frame = lv::counter{(runner_frame.get().value + frames_per_tick) % lap_frames};
  }
}

}  // namespace treadmill
