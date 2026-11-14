#include <lv/lv.hpp>
#include <string>
template <class T>
consteval T read_as(std::string_view text) {
  const auto v = *lv::json::value::parse(text);
  return lv::detail::read_token(lv::detail::token_of<T>{}, v, v);
}
constexpr auto d = read_as<lv::length>(R"("2!")");
