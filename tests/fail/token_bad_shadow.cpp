#include <lv/lv.hpp>
constexpr char doc[] = {
#embed "bad_shadow.json"
    , 0};
constexpr auto tok = lv::tokens<doc>;
