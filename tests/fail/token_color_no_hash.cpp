// A color that does not start with #.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"color": {"brand": {"$type": "color", "$value": "x123456"}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
