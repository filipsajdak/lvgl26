// A gradient of nine stops: eight at most.
#include "legacy_tokens.hpp"
constexpr char doc[] = R"({"fill": {"sky": {"$type": "gradient", "$value": [{"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}, {"color": "#102030", "position": 0}]}}})";
constexpr auto tok = lv::tokens<doc, legacy_tokens>;
