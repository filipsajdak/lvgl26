// A color of two components.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"color": {"brand": {"$type": "color", "$value": {"colorSpace": "srgb", "components": [0.2, 0.4]}}}})";
constexpr auto tok = lv::tokens<doc>;
