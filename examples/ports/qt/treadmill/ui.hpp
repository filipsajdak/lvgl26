#pragma once

// The design tokens of the panel and the sizes. Every file of the panel includes this and
// builds from these. The file has no state and no tree, so including it costs the reading of
// the token file and nothing else.

#include "sizes.hpp"

#include <lv/lv.hpp>

// The splash screen's wordmark is a 624 x 76 picture, larger than the default of 128 x 128.
// Every file of the panel includes this header, so they all agree.
template <>
inline constexpr lv::config lv::cfg<lv::override>{.compile_time_image_pixels = treadmill::largest_picture};

namespace treadmill {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace treadmill
