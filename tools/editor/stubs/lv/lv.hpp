#pragma once

// Editor stub of lv/lv.hpp. These headers declare the public API of lvgl26 as plain
// C++, so that clangd can complete, document and check code that only GCC 16 builds.
// tools/editor/generate.py copies them next to the generated ones.

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
#include <lv/detail/missing.hpp>

namespace lv {

/// The active screen of the default display.
obj screen_active();

}  // namespace lv
