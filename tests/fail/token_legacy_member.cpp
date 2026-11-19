// A member of a composite token written in the old form: the message names the token and the member.
#include <lv/lv.hpp>
constexpr char doc[] = R"({"t": {"$type": "typography", "body": {"$value": {"fontFamily": "Mono", "fontSize": "16px", "fontWeight": 400, "letterSpacing": {"value": 0, "unit": "px"}, "lineHeight": 1.5}}}})";
constexpr auto tok = lv::tokens<doc>;
