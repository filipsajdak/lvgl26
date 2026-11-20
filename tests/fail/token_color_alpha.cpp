// A color of a shadow with an alpha channel: a color token does not carry it.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"card": {"shadow": {"$type": "shadow", "$value": {"color": "#01020380", "offsetX": "0px", "offsetY": "2px", "blur": "4px", "spread": "0px"}}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
