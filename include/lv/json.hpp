#pragma once

// A small JSON reader that works at compile time and at run time.
// It allocates nothing: a value is a view of its text inside the document.
// Strings are returned as written, without unescaping.

#include <cstddef>
#include <optional>
#include <string_view>

namespace lv::json {

enum class kind { invalid, null, boolean, number, string, array, object };

namespace detail {

constexpr bool is_space(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

constexpr std::size_t skip_space(std::string_view s, std::size_t i) {
  while (i < s.size() && is_space(s[i])) ++i;
  return i;
}

inline constexpr std::size_t npos = std::string_view::npos;

// The index just past the string that starts at s[i] == '"', or npos.
constexpr std::size_t skip_string(std::string_view s, std::size_t i) {
  for (++i; i < s.size(); ++i) {
    if (s[i] == '\\') {
      ++i;
    } else if (s[i] == '"') {
      return i + 1;
    }
  }
  return npos;
}

// The index just past the value that starts at s[i], or npos if it is malformed.
constexpr std::size_t skip_value(std::string_view s, std::size_t i, int depth = 0) {
  if (i >= s.size() || depth > 64) return npos;
  const char c = s[i];
  if (c == '"') return skip_string(s, i);
  if (c == '{' || c == '[') {
    const char close = c == '{' ? '}' : ']';
    i = skip_space(s, i + 1);
    if (i < s.size() && s[i] == close) return i + 1;
    while (true) {
      if (c == '{') {
        if (i >= s.size() || s[i] != '"') return npos;
        i = skip_string(s, i);
        if (i == npos) return npos;
        i = skip_space(s, i);
        if (i >= s.size() || s[i] != ':') return npos;
        i = skip_space(s, i + 1);
      }
      i = skip_value(s, i, depth + 1);
      if (i == npos) return npos;
      i = skip_space(s, i);
      if (i >= s.size()) return npos;
      if (s[i] == close) return i + 1;
      if (s[i] != ',') return npos;
      i = skip_space(s, i + 1);
    }
  }
  for (std::string_view word : {std::string_view("true"), std::string_view("false"),
                                std::string_view("null")})
    if (s.substr(i).starts_with(word)) return i + word.size();
  std::size_t j = i;
  while (j < s.size() && (s[j] == '-' || s[j] == '+' || s[j] == '.' || s[j] == 'e' ||
                          s[j] == 'E' || (s[j] >= '0' && s[j] <= '9')))
    ++j;
  return j == i ? npos : j;
}

}  // namespace detail

class value;

struct member {
  std::string_view key;
  std::string_view text;  // the member's value
};

// A JSON value: a view of its text.
class value {
  std::string_view text_;

 public:
  constexpr value() = default;
  constexpr explicit value(std::string_view text) : text_(text) {}

  // Parses a whole document. Empty if it is not one well-formed JSON value.
  static constexpr std::optional<value> parse(std::string_view doc) {
    const auto begin = detail::skip_space(doc, 0);
    const auto end = detail::skip_value(doc, begin);
    if (end == detail::npos || detail::skip_space(doc, end) != doc.size()) return std::nullopt;
    return value{doc.substr(begin, end - begin)};
  }

  // The value that starts at `offset` in `doc`. The offset must come from a value
  // of the same document.
  static constexpr value at_offset(std::string_view doc, std::size_t offset)
    pre(offset < doc.size())
  {
    return value{doc.substr(offset, detail::skip_value(doc, offset) - offset)};
  }

  constexpr std::string_view text() const { return text_; }

  constexpr kind type() const {
    if (text_.empty()) return kind::invalid;
    switch (text_.front()) {
      case '{': return kind::object;
      case '[': return kind::array;
      case '"': return kind::string;
      case 't':
      case 'f': return kind::boolean;
      case 'n': return kind::null;
      default: return kind::number;
    }
  }

  constexpr bool is(kind k) const { return type() == k; }

  // The characters between the quotes, as written.
  constexpr std::string_view string() const {
    return is(kind::string) ? text_.substr(1, text_.size() - 2) : std::string_view{};
  }

  constexpr bool boolean() const { return text_ == "true"; }

  constexpr double number() const {
    std::size_t i = 0;
    const bool negative = !text_.empty() && text_[0] == '-';
    if (negative) ++i;
    double v = 0;
    for (; i < text_.size() && text_[i] >= '0' && text_[i] <= '9'; ++i) v = v * 10 + (text_[i] - '0');
    if (i < text_.size() && text_[i] == '.') {
      double scale = 0.1;
      for (++i; i < text_.size() && text_[i] >= '0' && text_[i] <= '9'; ++i, scale /= 10)
        v += (text_[i] - '0') * scale;
    }
    if (i < text_.size() && (text_[i] == 'e' || text_[i] == 'E')) {
      ++i;
      const bool down = i < text_.size() && text_[i] == '-';
      if (i < text_.size() && (text_[i] == '-' || text_[i] == '+')) ++i;
      int e = 0;
      for (; i < text_.size() && text_[i] >= '0' && text_[i] <= '9'; ++i) e = e * 10 + (text_[i] - '0');
      for (; e > 0; --e) v = down ? v / 10 : v * 10;
    }
    return negative ? -v : v;
  }

  // How many members an object has, or elements an array has.
  constexpr std::size_t size() const {
    std::size_t n = 0;
    for (std::size_t i = first(); i != detail::npos; i = next(i)) ++n;
    return n;
  }

  // Member number `index` of an object, in document order.
  constexpr member member_at(std::size_t index) const {
    std::size_t i = first();
    for (; index > 0 && i != detail::npos; --index) i = next(i);
    return i == detail::npos ? member{} : read(i);
  }

  // Element number `index` of an array.
  constexpr value at(std::size_t index) const {
    std::size_t i = first();
    for (; index > 0 && i != detail::npos; --index) i = next(i);
    if (i == detail::npos) return {};
    return value{text_.substr(i, detail::skip_value(text_, i) - i)};
  }

  // The member of an object called `key`. Invalid if there is none.
  constexpr value operator[](std::string_view key) const {
    if (!is(kind::object)) return {};
    for (std::size_t i = first(); i != detail::npos; i = next(i))
      if (const auto m = read(i); m.key == key) return value{m.text};
    return {};
  }

  constexpr explicit operator bool() const { return type() != kind::invalid; }

 private:
  // Index of the first member or element, or npos when there is none.
  constexpr std::size_t first() const {
    if (!is(kind::object) && !is(kind::array)) return detail::npos;
    const auto i = detail::skip_space(text_, 1);
    return (text_[i] == '}' || text_[i] == ']') ? detail::npos : i;
  }

  // Index of the member or element after the one at `i`, or npos.
  constexpr std::size_t next(std::size_t i) const {
    if (is(kind::object)) {
      i = detail::skip_space(text_, detail::skip_string(text_, i));
      i = detail::skip_space(text_, i + 1);  // past ':'
    }
    i = detail::skip_space(text_, detail::skip_value(text_, i));
    return text_[i] == ',' ? detail::skip_space(text_, i + 1) : detail::npos;
  }

  constexpr member read(std::size_t i) const {
    const auto key_end = detail::skip_string(text_, i);
    const auto v = detail::skip_space(text_, detail::skip_space(text_, key_end) + 1);
    return {text_.substr(i + 1, key_end - i - 2), text_.substr(v, detail::skip_value(text_, v) - v)};
  }
};

}  // namespace lv::json
