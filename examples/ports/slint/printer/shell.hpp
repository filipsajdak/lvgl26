#pragma once

// The window: the side bar, and the places where the pages are shown.

#include "sizes.hpp"

#include <lv/lv.hpp>

namespace printer {

// Builds the window on `screen` and starts what follows the state. False if LVGL has no memory.
bool start(lv::obj screen);

}  // namespace printer
