#include <lv/lv.hpp>
constexpr auto root = *lv::json::value::parse(
    R"({"g": {"a": {"$value": "{g.b}"}, "b": {"$value": "{g.a}"}}})");
constexpr auto d = lv::detail::follow_alias(root, "{g.a}", 0);
