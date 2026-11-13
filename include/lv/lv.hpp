#pragma once

#include <lv/owned.hpp>
#include <lv/tree.hpp>
#include <lv/view.hpp>

namespace lv {

// The active screen of the default display.
inline obj screen_active() { return obj{lv_screen_active()}; }

}  // namespace lv
