// A color of ten digits.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"color": {"brand": {"$type": "color", "$value": "#1122334455"}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
