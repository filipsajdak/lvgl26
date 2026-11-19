// A color written "#3366ff": the form before Design Tokens Format 2025.10 is an error unless the settings allow it.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"c": {"$type": "color", "ink": {"$value": "#3366ff"}}})";
constexpr auto tok = lv::tokens<doc>;
