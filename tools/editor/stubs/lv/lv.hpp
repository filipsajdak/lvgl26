#pragma once

// Editor stub of lv/lv.hpp. These headers declare the public API of lvgl26 as plain
// C++, so that clangd can complete, document and check code that only GCC 16 builds.
// tools/editor/generate.py copies them next to the generated ones.

#include <lv/animate.hpp>
#include <lv/application.hpp>
#include <lv/computed.hpp>
#include <lv/display.hpp>
#include <lv/encoded_image.hpp>
#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/list.hpp>
#include <lv/look.hpp>
#include <lv/match.hpp>
#include <lv/owned.hpp>
#include <lv/repeat.hpp>
#include <lv/runtime.hpp>
#include <lv/scaled_font.hpp>
#include <lv/state.hpp>
#include <lv/style.hpp>
#include <lv/task.hpp>
#include <lv/theme.hpp>
#include <lv/tokens.hpp>
#include <lv/tree.hpp>
#include <lv/view.hpp>

namespace lv {

/// The active screen of the default display.
obj screen_active();

}  // namespace lv
