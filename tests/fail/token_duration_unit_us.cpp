// A duration in microseconds.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"time": {"fast": {"$type": "duration", "$value": {"value": 2, "unit": "us"}}}})";
constexpr auto tok = lv::tokens<doc>;
