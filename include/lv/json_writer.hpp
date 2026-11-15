#pragma once

// Writing JSON text into a sink, piece by piece. The counterpart of lv/json.hpp,
// and like it plain C++23 with no contracts and no reflection.
//
//   std::string text;
//   auto sink = [&](std::string_view piece) { text += piece; };
//   lv::json::write_string(sink, "say \"hi\"");   // "say \"hi\""
//   lv::json::write_number(sink, -12);            // -12
//
// A sink is anything callable with a std::string_view: it may append to a buffer,
// write to a file or count the characters. Nothing is allocated here.

#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <functional>
#include <string_view>

namespace lv::json {

// Takes the pieces of a text, in order.
template <class S>
concept sink = std::invocable<S&, std::string_view>;

namespace detail {

// The two-character escape of `c` ("\n" for a line feed), or nothing.
constexpr std::string_view short_escape(char c) {
  switch (c) {
    case '"': return "\\\"";
    case '\\': return "\\\\";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default: return {};
  }
}

constexpr bool is_control(char c) { return static_cast<unsigned char>(c) < 0x20; }

// Writes one character of a string the way JSON needs it.
template <sink S>
void write_escaped(S& out, char c) {
  if (const auto escape = short_escape(c); !escape.empty()) {
    std::invoke(out, escape);
  } else if (is_control(c)) {
    constexpr std::string_view digits = "0123456789abcdef";
    const auto u = static_cast<unsigned char>(c);
    const std::array<char, 6> text{'\\', 'u', '0', '0', digits[u >> 4], digits[u & 15]};
    std::invoke(out, std::string_view{text.data(), text.size()});
  } else {
    std::invoke(out, std::string_view{&c, 1});
  }
}

}  // namespace detail

// Writes `text` as a JSON string: in quotes, with quotes, backslashes and control
// characters escaped. Other bytes pass as they are, so UTF-8 stays UTF-8.
template <sink S>
void write_string(S& out, std::string_view text) {
  std::invoke(out, std::string_view{"\""});
  std::size_t plain = 0;  // where the run of characters that need no escape starts
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (detail::short_escape(text[i]).empty() && !detail::is_control(text[i])) continue;
    std::invoke(out, text.substr(plain, i - plain));
    detail::write_escaped(out, text[i]);
    plain = i + 1;
  }
  std::invoke(out, text.substr(plain));
  std::invoke(out, std::string_view{"\""});
}

// Writes a whole number.
template <sink S>
void write_number(S& out, std::int64_t number) {
  std::array<char, 24> text{};
  const auto end = std::to_chars(text.data(), text.data() + text.size(), number).ptr;
  std::invoke(out, std::string_view{text.data(), static_cast<std::size_t>(end - text.data())});
}

}  // namespace lv::json
