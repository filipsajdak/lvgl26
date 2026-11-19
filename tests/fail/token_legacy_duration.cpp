// A duration written "200ms".
#include <lv/lv.hpp>
constexpr char doc[] = R"({"t": {"$type": "duration", "fast": {"$value": "200ms"}}})";
constexpr auto tok = lv::tokens<doc>;
