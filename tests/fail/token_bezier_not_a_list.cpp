#include <lv/lv.hpp>
#include <string>
template <class T>
consteval T read_as(std::string_view text) {
  const auto v = *lv::json::value::parse(text);
  return lv::detail::read_token(lv::detail::read_as<T>{}, v, v);
}
constexpr auto d = read_as<lv::bezier>("0.5");
