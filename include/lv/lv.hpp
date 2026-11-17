#pragma once

#include <lv/animate.hpp>
#include <lv/application.hpp>
#include <lv/computed.hpp>
#include <lv/display.hpp>
#include <lv/encoded_image.hpp>
#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/list.hpp>
#include <lv/match.hpp>
#include <lv/owned.hpp>
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

// The active screen of the default display.
inline obj screen_active() { return obj{lv_screen_active()}; }

}  // namespace lv
