// A font weight that is no number and no name.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"type": {"weight": {"$type": "fontWeight", "$value": "thin"}}})";
constexpr auto tok = lv::tokens<doc>;
