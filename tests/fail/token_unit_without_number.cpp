// A dimension that is only a unit.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"space": {"gap": {"$type": "dimension", "$value": "px"}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
