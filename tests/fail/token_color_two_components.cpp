#include <lv/lv.hpp>
constexpr auto v = *lv::json::value::parse(R"({"components": [1, 0]})");
constexpr auto c = lv::detail::read_token(lv::detail::token_of<lv_color_t>{}, v, v);
