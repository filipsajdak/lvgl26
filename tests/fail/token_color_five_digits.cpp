// A color of five digits.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"color": {"brand": {"$type": "color", "$value": "#12345"}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
