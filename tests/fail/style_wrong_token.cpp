#include <lv/lv.hpp>
constexpr char doc[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<doc>;
constexpr auto s = lv::style({.radius = tok.color.primary});
