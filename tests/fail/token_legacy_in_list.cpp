// A shadow list: each shadow is checked, not only the first.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"s": {"$type": "shadow", "lift": {"$value": [
  {"color": {"colorSpace": "srgb", "components": [0, 0, 0]}, "offsetX": {"value": 0, "unit": "px"}, "offsetY": {"value": 1, "unit": "px"}, "blur": {"value": 2, "unit": "px"}, "spread": {"value": 0, "unit": "px"}},
  {"color": {"colorSpace": "srgb", "components": [0, 0, 0]}, "offsetX": "0px", "offsetY": {"value": 1, "unit": "px"}, "blur": {"value": 2, "unit": "px"}, "spread": {"value": 0, "unit": "px"}}]}}})";
constexpr auto tok = lv::tokens<doc>;
