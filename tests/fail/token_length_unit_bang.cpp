// A dimension with a unit that is no unit.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"space": {"gap": {"$type": "dimension", "$value": {"value": 8, "unit": "!"}}}})";
constexpr auto tok = lv::tokens<doc>;
