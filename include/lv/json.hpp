#pragma once

// A small JSON reader that works at compile time and at run time.
// It is plain C++23 with no contracts and no reflection, so that Clang can compile
// it too: its fuzz target (fuzz/json_reader.cpp) is built with libFuzzer.
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

constexpr std::size_t skip_value(std::string_view s, std::size_t i, int depth = 0);

constexpr bool is_digit(char c) { return c >= '0' && c <= '9'; }

// How many characters at the start of `s` belong to a plain decimal number.
constexpr std::size_t number_length(std::string_view s) {
  std::size_t i = 0;
  while (i < s.size() && (s[i] == '-' || s[i] == '.' || is_digit(s[i]))) ++i;
  return i;
}

// The index just past the digits that start at s[i].
constexpr std::size_t skip_digits(std::string_view s, std::size_t i) {
  while (i < s.size() && is_digit(s[i])) ++i;
  return i;
}

// The index just past an optional ".digits" at s[i], or npos if the dot has no digits.
constexpr std::size_t skip_fraction(std::string_view s, std::size_t i) {
  if (i >= s.size() || s[i] != '.') return i;
  const auto end = skip_digits(s, i + 1);
  return end == i + 1 ? npos : end;
}

// The index just past an optional exponent at s[i], or npos if it has no digits.
constexpr std::size_t skip_exponent(std::string_view s, std::size_t i) {
  if (i >= s.size() || (s[i] != 'e' && s[i] != 'E')) return i;
  ++i;
  if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
  const auto end = skip_digits(s, i);
  return end == i ? npos : end;
}

// The index just past the number that starts at s[i], or npos if there is none.
// The JSON grammar exactly: -? (0 | [1-9][0-9]*) (.[0-9]+)? ([eE][+-]?[0-9]+)?
constexpr std::size_t skip_number(std::string_view s, std::size_t i) {
  if (i < s.size() && s[i] == '-') ++i;
  const auto whole = skip_digits(s, i);
  if (whole == i) return npos;                          // no digits
  if (s[i] == '0' && whole > i + 1) return npos;        // a leading zero
  const auto fraction = skip_fraction(s, whole);
  return fraction == npos ? npos : skip_exponent(s, fraction);
}

// The index just past true, false or null at s[i], or npos.
constexpr std::size_t skip_word(std::string_view s, std::size_t i) {
  for (std::string_view word : {std::string_view("true"), std::string_view("false"),
                                std::string_view("null")})
    if (s.substr(i).starts_with(word)) return i + word.size();
  return npos;
}

// The index of the value of the member whose key starts at s[i], or npos.
constexpr std::size_t skip_key(std::string_view s, std::size_t i) {
  if (i >= s.size() || s[i] != '"') return npos;
  i = skip_space(s, skip_string(s, i));  // npos stays npos
  if (i >= s.size() || s[i] != ':') return npos;
  return skip_space(s, i + 1);
}

// After one element that ends at s[i]: the index of the next element, the index
// just past the closing character (with `closed` set), or npos.
constexpr std::size_t after_element(std::string_view s, std::size_t i, char close, bool& closed) {
  i = skip_space(s, i);
  if (i >= s.size()) return npos;
  closed = s[i] == close;
  if (closed) return i + 1;
  return s[i] == ',' ? skip_space(s, i + 1) : npos;
}

// The index just past one element of a container (a member of an object, or a
// value of an array) that starts at s[i], or npos.
constexpr std::size_t skip_element(std::string_view s, std::size_t i, bool object, int depth) {
  if (object) i = skip_key(s, i);
  return i == npos ? npos : skip_value(s, i, depth + 1);
}

// The index just past the object or array that opens at s[i], or npos.
constexpr std::size_t skip_container(std::string_view s, std::size_t i, int depth) {
  const bool object = s[i] == '{';
  const char close = object ? '}' : ']';
  i = skip_space(s, i + 1);
  if (i < s.size() && s[i] == close) return i + 1;
  for (bool closed = false; !closed && i != npos;) {
    i = skip_element(s, i, object, depth);
    if (i != npos) i = after_element(s, i, close, closed);
  }
  return i;
}

// The index just past the value that starts at s[i], or npos if it is malformed.
constexpr std::size_t skip_value(std::string_view s, std::size_t i, int depth) {
  if (i >= s.size() || depth > 64) return npos;
  if (s[i] == '"') return skip_string(s, i);
  if (s[i] == '{' || s[i] == '[') return skip_container(s, i, depth);
  if (const auto word = skip_word(s, i); word != npos) return word;
  return skip_number(s, i);
}

// A run of digits read as a whole number, advancing i past it.
constexpr double read_digits(std::string_view s, std::size_t& i) {
  double v = 0;
  for (; i < s.size() && is_digit(s[i]); ++i) v = v * 10 + (s[i] - '0');
  return v;
}

// The digits after a decimal point, as a value below one, advancing i past them.
constexpr double read_fraction(std::string_view s, std::size_t& i) {
  if (i >= s.size() || s[i] != '.') return 0;
  double v = 0, scale = 0.1;
  for (++i; i < s.size() && is_digit(s[i]); ++i, scale /= 10) v += (s[i] - '0') * scale;
  return v;
}

// Reads an optional sign at s[i], advancing i past it. True for a minus.
constexpr bool read_minus(std::string_view s, std::size_t& i) {
  if (i >= s.size() || (s[i] != '-' && s[i] != '+')) return false;
  return s[i++] == '-';
}

// A run of digits as an exponent, advancing i past it. It stops counting at 999:
// any larger exponent already gives zero or infinity, and an int must not overflow.
constexpr int read_exponent_digits(std::string_view s, std::size_t& i) {
  int e = 0;
  for (; i < s.size() && is_digit(s[i]); ++i)
    if (e < 100) e = e * 10 + (s[i] - '0');
  return e;
}

// The factor of an exponent part (e-3 gives 0.001), advancing i past it. 1 if there is none.
constexpr double read_exponent(std::string_view s, std::size_t& i) {
  if (i >= s.size() || (s[i] != 'e' && s[i] != 'E')) return 1;
  ++i;
  const bool down = read_minus(s, i);
  double factor = 1;
  for (int e = read_exponent_digits(s, i); e > 0; --e) factor = down ? factor / 10 : factor * 10;
  return factor;
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
    const bool negative = detail::read_minus(text_, i);
    double v = detail::read_digits(text_, i);
    v += detail::read_fraction(text_, i);
    v *= detail::read_exponent(text_, i);
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
    if (!is(kind::object)) return {};
    std::size_t i = first();
    for (; index > 0 && i != detail::npos; --index) i = next(i);
    return i == detail::npos ? member{} : read(i);
  }

  // Element number `index` of an array.
  constexpr value at(std::size_t index) const {
    if (!is(kind::array)) return {};  // an object's "elements" would be its keys
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
