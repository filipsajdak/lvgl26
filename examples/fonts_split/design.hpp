#pragma once

// The design of the project: its token file, named once. Every file of the program includes it.
// It holds no font file: the files that draw the fonts embed them (font_*.cpp).
#include <lv/look.hpp>
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};
