// A gradient that is a number.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"fill": {"sky": {"$type": "gradient", "$value": 5}}})";
constexpr auto tok = lv::tokens<doc>;
