// A duration in hours.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"time": {"fast": {"$type": "duration", "$value": {"value": 2, "unit": "h"}}}})";
constexpr auto tok = lv::tokens<doc>;
