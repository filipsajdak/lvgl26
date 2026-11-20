#pragma once

// The core of lvgl26: what nearly every program writes. A feature that costs a file compile time
// whether or not it uses it has a header of its own (docs/which-header.md); lv/all.hpp includes them all.

#include <lv/animate.hpp>
#include <lv/application.hpp>
#include <lv/computed.hpp>
#include <lv/display.hpp>
#include <lv/match.hpp>
#include <lv/options.hpp>
#include <lv/owned.hpp>
#include <lv/state.hpp>
#include <lv/style.hpp>
#include <lv/task.hpp>
#include <lv/tokens.hpp>
#include <lv/tree.hpp>
#include <lv/view.hpp>

namespace lv {

// The active screen of the default display.
inline obj screen_active() { return obj{lv_screen_active()}; }

}  // namespace lv
