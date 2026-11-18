#pragma once

// The registration of a project, in a header that every file includes: the way lv::look is meant
// to be used. tests/look_link_main.cpp and tests/look_link_cards.cpp include it and link together.

#include <lv/lv.hpp>

namespace link_project {

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace link_project

template <>
inline constexpr lv::design_of<decltype(link_project::tok)> lv::design<lv::override>{
    .tokens = link_project::tok, .type = lv::typeface({{"Barlow", 500, link_project::barlow}})};

// Defined in look_link_cards.cpp.
lv::obj mount_card(lv::obj parent);
