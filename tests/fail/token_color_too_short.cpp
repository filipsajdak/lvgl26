// A color with no digits.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"color": {"brand": {"$type": "color", "$value": "#"}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
