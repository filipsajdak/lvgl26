#pragma once

// The scenes of the printer panel that the screenshot tool and the test both visit: each is
// a page, brought up by changing the state that the buttons change.

#include "state.hpp"

#include <lv/lv.hpp>

#include <array>

namespace printer {

struct scene {
  const char* name;
  page shown;
  subpage over;
};

inline constexpr std::array scenes{
  scene{"home", page::home, subpage::none},
  scene{"settings", page::settings, subpage::none},
  scene{"ink", page::ink, subpage::none},
  scene{"print", page::home, subpage::print},
  scene{"scan", page::home, subpage::scan},
  scene{"copy", page::home, subpage::copy},
  scene{"usb", page::home, subpage::usb},
};

// How long LVGL runs after a change: a page slides in over 125 ms and the ink rises in 750 ms.
inline constexpr int settle_steps = 120;
inline constexpr int step_ms      = 10;

// Shows the scene and lets the animations finish.
inline void play(lv::application& app, const scene& s) {
  active_subpage = subpage::none;
  active_page    = s.shown;
  active_subpage = s.over;
  for (int step = 0; step < settle_steps; ++step) {
    app.advance(step_ms * lv::ms);
    app.step();
  }
}

}  // namespace printer
