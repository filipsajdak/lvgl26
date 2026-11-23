#pragma once

// The registration of a project whose fonts are drawn in files of their own: the tokens only.
// No file holds the font files; font_label.cpp and font_small.cpp embed them.
// tests/fonts_split_main.cpp names the fonts and links with both.

#include <lv/look.hpp>
#include <lv/lv.hpp>

namespace split_project {

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace split_project

template <>
inline constexpr lv::design_of<decltype(split_project::tok)> lv::design<lv::override>{.tokens = split_project::tok};
