#include <lv/lv.hpp>
constexpr char doc[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<doc>;
constexpr auto c = tok.color.primry;
