// A dimension written "8px": the message names the token, the form found and the form expected.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"s": {"$type": "dimension", "gap": {"$value": "8px"}}})";
constexpr auto tok = lv::tokens<doc>;
