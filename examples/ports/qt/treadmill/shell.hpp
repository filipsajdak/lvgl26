#pragma once

// The window: the main screen, and the splash screen over it for the first seconds.

#include "sizes.hpp"

#include <lv/lv.hpp>

namespace treadmill {

// Builds the window on `screen` and starts what follows the state. False if LVGL has no memory.
bool start(lv::obj screen);

}  // namespace treadmill
