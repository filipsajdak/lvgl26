#include <lv/lv.hpp>
#include <string>
consteval double through_aliases(int links) {  // at most 9, so that a name is one digit
  const auto digit = [](int i) { return std::string(1, static_cast<char>('0' + i)); };
  std::string doc = "{\"g\": {";
  for (int i = 0; i < links; ++i)
    doc += "\"t" + digit(i) + "\": {\"$value\": \"{g.t" + digit(i + 1) + "}\"}, ";
  doc += "\"t" + digit(links) + "\": {\"$type\": \"number\", \"$value\": 7}}}";
  const auto root = *lv::json::value::parse(doc);
  return lv::detail::follow_alias(root, "{g.t0}", 0).value.number();
}
constexpr auto d = through_aliases(9);
