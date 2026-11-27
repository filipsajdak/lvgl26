#pragma once

// Everything of lvgl26 that is not a platform or a tool: the core (lv/lv.hpp) and every opt-in
// header. A file that includes it pays the compile time of all of them; docs/which-header.md says
// which header each name needs. Signal (lv/signal.hpp, lv/signal_type.hpp, lv/signal_lamps.hpp,
// lv/signal_motion.hpp), the host (lv/host.hpp), threads (lv/ui_scheduler.hpp), the agent kit
// (lv/mcp.hpp) and PNG output (lv/png.hpp) are included one by one.

#include <lv/encoded_image.hpp>
#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/list.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/one_of.hpp>
#include <lv/shown.hpp>
#include <lv/pointer.hpp>
#include <lv/repeat.hpp>
#include <lv/runtime.hpp>
#include <lv/scaled_font.hpp>
#include <lv/theme.hpp>
