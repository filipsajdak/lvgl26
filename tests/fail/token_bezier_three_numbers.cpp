// A cubic bezier of three numbers.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"ease": {"out": {"$type": "cubicBezier", "$value": [0.1, 0.2, 0.3]}}})";
constexpr auto tok = lv::tokens<doc>;
