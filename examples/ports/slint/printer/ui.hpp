#pragma once

// The design tokens of the panel and the sizes. Every page of the panel includes this and
// builds its tree from these. The file has no state and no tree, so including it costs the
// reading of the token file and nothing else.

#include "icons.hpp"
#include "sizes.hpp"

#include <lv/lv.hpp>

namespace printer {

using namespace lv::units;

inline constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
inline constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace printer
