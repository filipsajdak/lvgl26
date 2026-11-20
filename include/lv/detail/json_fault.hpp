#pragma once

// Where a JSON document stops being well-formed, and what was expected there, for a message. It runs
// after json::value::parse has said that the document is not one value, and never on the way that works.

#include <lv/detail/api.hpp>
#include <lv/json.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace lv::json::detail {

struct fault {
  std::size_t at = npos;
  std::string_view expected;
};

inline constexpr int deepest_nesting = 64;  // as skip_value

constexpr fault value_fault(std::string_view s, std::size_t& i, int depth);

// After a member or an element that ends at s[i]: the fault, or nothing with `closed` set when the container ends.
constexpr fault after_fault(std::string_view s, std::size_t& i, char close, bool& closed) {
  i = skip_space(s, i);
  if (i >= s.size() || (s[i] != ',' && s[i] != close))
    return {i, close == '}' ? "',' or '}' after the value" : "',' or ']' after the value"};
  closed = s[i] == close;
  i = skip_space(s, i + 1);
  return {};
}

// The key and the colon of a member that starts at s[i].
constexpr fault key_fault(std::string_view s, std::size_t& i, bool first) {
  if (i >= s.size() || s[i] != '"') return {i, first ? "a key in quotes or '}'" : "a key in quotes"};
  const auto end = skip_string(s, i);
  if (end == npos) return {i, "the closing quote of this string"};
  i = skip_space(s, end);
  if (i >= s.size() || s[i] != ':') return {i, "':' after the key"};
  i = skip_space(s, i + 1);
  return {};
}

// One member of an object (its key, the colon and the value) or one element of a list, at s[i].
constexpr fault element_fault(std::string_view s, std::size_t& i, bool object, bool first, int depth) {
  if (object)
    if (const auto f = key_fault(s, i, first); f.at != npos) return f;
  return value_fault(s, i, depth + 1);
}

// The object or list that opens at s[i].
constexpr fault container_fault(std::string_view s, std::size_t& i, int depth) {
  const bool object = s[i] == '{';
  const char close = object ? '}' : ']';
  i = skip_space(s, i + 1);
  if (i < s.size() && s[i] == close) return ++i, fault{};
  bool closed = false;
  for (bool first = true; !closed; first = false) {
    if (const auto f = element_fault(s, i, object, first, depth); f.at != npos) return f;
    if (const auto f = after_fault(s, i, close, closed); f.at != npos) return f;
  }
  return {};
}

// A string, a word or a number that starts at s[i].
constexpr fault scalar_fault(std::string_view s, std::size_t& i) {
  const bool quoted = s[i] == '"';
  const auto end = quoted ? skip_string(s, i) : (skip_word(s, i) != npos ? skip_word(s, i) : skip_number(s, i));
  if (end == npos)
    return {i, quoted ? "the closing quote of this string" : "a value: a string, a number, true, false, null, an object or a list"};
  i = end;
  return {};
}

// The value that starts at s[i]; i moves past it.
constexpr fault value_fault(std::string_view s, std::size_t& i, int depth) {
  if (i >= s.size()) return {i, "a value"};
  if (depth > deepest_nesting) return {i, "fewer levels: a document nests 64 at most"};
  if (s[i] == '{' || s[i] == '[') return container_fault(s, i, depth);
  return scalar_fault(s, i);
}

// The first place where `s` is not one JSON value.
constexpr fault find_fault(std::string_view s) {
  std::size_t i = skip_space(s, 0);
  if (const auto f = value_fault(s, i, 0); f.at != npos) return f;
  i = skip_space(s, i);
  return i < s.size() ? fault{i, "the end of the file"} : fault{};
}

// "At line 1, column 13 it expects a key in quotes or '}', and the file ends there."
consteval std::string describe_fault(std::string_view s, const fault& f) {
  std::size_t line = 1, column = 1;
  for (std::size_t i = 0; i < f.at; ++i) {
    if (s[i] == '\n') ++line, column = 1;
    else ++column;
  }
  const bool ends = f.at >= s.size();
  return lv::detail::join({"At line ", lv::detail::decimal(line), ", column ", lv::detail::decimal(column), " it expects ",
                           f.expected, ", and ", ends ? "the file ends there" : "it has '", ends ? "" : s.substr(f.at, 1),
                           ends ? "" : "' there", "."});
}

}  // namespace lv::json::detail
