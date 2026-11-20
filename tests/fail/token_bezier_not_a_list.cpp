// A cubic bezier that is a number.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"ease": {"out": {"$type": "cubicBezier", "$value": 5}}})";
constexpr auto tok = lv::tokens<doc>;
