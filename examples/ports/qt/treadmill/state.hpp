#pragma once

// What the pages read and write. Each is one object for the whole program: a page binds to it,
// and the buttons change it. The rules of the original (how the numbers move) are in logic.cpp.

#include "ui.hpp"

#include <lv/lv.hpp>

#include <mp-units/systems/international.h>
#include <mp-units/systems/si.h>

namespace treadmill {

// The speed counts in tenths of a mile an hour, as the original's buttons step it.
inline constexpr struct tenth_mph final
    : mp_units::named_unit<"0.1 mi/h", mp_units::mag_ratio<1, 10> * mp_units::international::mile / mp_units::si::hour> {
} tenth_mph;

using speed_t = mp_units::quantity<tenth_mph, int>;
using seconds_t = mp_units::quantity<mp_units::si::second, int>;

enum class look { light, dark, sporty };

inline constinit lv::state<look> active_look{look::light};

// The readings.
inline constinit lv::state<lv::counter> heart_rate{lv::counter{0}};
inline constinit lv::state<lv::counter> elevation{lv::counter{0}};
inline constinit lv::state<lv::counter> incline{lv::counter{0}};  // percent
inline constinit lv::state<speed_t> speed{speed_t{}};
inline constinit lv::state<seconds_t> elapsed{seconds_t{}};
inline constinit lv::state<seconds_t> pace{seconds_t{}};  // for one mile

// What the machine is doing.
inline constinit lv::state<bool> clock_running{false};  // the readings count
inline constinit lv::state<bool> auto_mode{false};      // the switch is on "auto"
inline constinit lv::state<bool> paused{false};
inline constinit lv::state<bool> runner_moving{false};  // the dot goes round the track
inline constinit lv::state<lv::counter> runner_frame{lv::counter{0}};

// The readings as text, which the labels show.
inline constinit lv::state<const char*> time_text{"00:00"};
inline constinit lv::state<const char*> pace_text{"0:00"};
inline constinit lv::state<const char*> speed_text{"0.0"};

// What the buttons do.
void start();
void pause();
void stop();
void toggle_mode();
void flatter();
void steeper();
void slower();
void faster();
template <look L>
void choose_look() {
  active_look = L;
}

// The switch shows "manual" while auto mode is off.
constexpr bool is_manual(bool automatic) { return !automatic; }
inline constinit lv::computed manual_mode{&is_manual, auto_mode};

// Where the dot is for a frame of the track's timeline.
struct spot {
  int x;
  int y;
};
spot runner_at(int frame);

// The frames of one lap, and how many the dot passes in a tick of its task.
inline constexpr int lap_frames = 1000;

// Starts the effects that write the text states. False if LVGL has no memory.
bool start_readouts();

// The clocks of the machine: the counting of the readings, and the automatic mode. Both tick once a second.
lv::task<> run_clock();
lv::task<> run_auto();

// Moves a dot along the track while the machine runs; a page spawns it for its own dot.
lv::task<> follow_runner(lv::view<"image"> runner);

}  // namespace treadmill
