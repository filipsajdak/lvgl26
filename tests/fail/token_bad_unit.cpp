#include <lv/lv.hpp>
constexpr char doc[] = {
#embed "bad_unit.json"
    , 0};
constexpr auto tok = lv::tokens<doc>;
