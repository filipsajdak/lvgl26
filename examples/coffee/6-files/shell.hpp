#pragma once

// The frame of the machine: the place where its pages are shown.

#include <lv/lv.hpp>

namespace coffee {

// The machine's screen, landscape.
inline constexpr int width  = 480;
inline constexpr int height = 320;

// Builds the frame on `screen`, with the page of the current step in it. False if LVGL has no
// memory for it.
bool start(lv::obj screen);

}  // namespace coffee
