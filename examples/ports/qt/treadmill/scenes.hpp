#pragma once

// The scenes of the panel that the screenshot tool and the test both visit: each is a moment,
// reached by pressing what a finger presses and waiting.

#include "shell.hpp"
#include "state.hpp"

#include <lv/lv.hpp>

#include <array>

namespace treadmill {

struct scene {
  const char* name;
  void (*act)();  // what the finger does, or null
  int wait_ms;    // how long LVGL runs after that
};

// The splash screen has slid in. Then it goes at five seconds, and the main screen shows.
inline constexpr int splash_shown_ms = 1200;
inline constexpr int splash_over_ms  = 4000;

// A run on a slope: START, a faster pace and a steeper slope, and three seconds of counting.
inline constexpr int faster_presses  = 85;  // 8.5 mph
inline constexpr int steeper_presses = 8;   // 8 percent
inline constexpr int run_ms          = 3200;
inline constexpr int look_ms         = 100;

inline void run_on_a_slope() {
  start();
  for (int press = 0; press < faster_presses; ++press) faster();
  for (int press = 0; press < steeper_presses; ++press) steeper();
}

inline constexpr std::array scenes{
  scene{"splash", nullptr, splash_shown_ms},
  scene{"light", nullptr, splash_over_ms},
  scene{"running", &run_on_a_slope, run_ms},
  scene{"dark", &choose_look<look::dark>, look_ms},
  scene{"sporty", &choose_look<look::sporty>, look_ms},
  scene{"auto", &toggle_mode, run_ms},
};

// How often LVGL is told the time while a scene runs.
inline constexpr int step_ms = 10;

// Does what the scene says and lets LVGL run.
inline void play(lv::application& app, const scene& s) {
  if (s.act != nullptr) s.act();
  for (int elapsed_ms = 0; elapsed_ms < s.wait_ms; elapsed_ms += step_ms) {
    app.advance(step_ms * lv::ms);
    app.step();
  }
}

}  // namespace treadmill
