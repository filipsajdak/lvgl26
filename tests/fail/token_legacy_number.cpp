// A bare number for a dimension: pixels before the standard, an object now.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"s": {"$type": "dimension", "gap": {"$value": 8}}})";
constexpr auto tok = lv::tokens<doc>;
