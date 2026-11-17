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
#include <cstddef>
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

// ---- UTF-8 (The Unicode Standard, chapter 3.9, table 3-7) ----------------------

constexpr unsigned char byte(char c) { return static_cast<unsigned char>(c); }

// The first byte that is not ASCII: from here on a byte belongs to a longer sequence.
inline constexpr unsigned char first_non_ascii = 0x80;

// The range of a byte that continues a sequence.
inline constexpr unsigned char continuation_low = 0x80;
inline constexpr unsigned char continuation_high = 0xbf;

// The most bytes a character takes.
inline constexpr std::size_t longest_sequence = 4;

// What may follow a first byte in `first`..`last`: `length` bytes in all, the second
// in `second_low`..`second_high`, the others continuation bytes. The narrow second
// ranges keep out overlong forms, surrogates and anything above U+10FFFF.
struct utf8_lead {
  unsigned char first;
  unsigned char last;
  std::size_t length;
  unsigned char second_low;
  unsigned char second_high;
};

inline constexpr std::array<utf8_lead, 8> utf8_leads{{
    {0xc2, 0xdf, 2, continuation_low, continuation_high},
    {0xe0, 0xe0, 3, 0xa0, continuation_high},
    {0xe1, 0xec, 3, continuation_low, continuation_high},
    {0xed, 0xed, 3, continuation_low, 0x9f},
    {0xee, 0xef, 3, continuation_low, continuation_high},
    {0xf0, 0xf0, 4, 0x90, continuation_high},
    {0xf1, 0xf3, 4, continuation_low, continuation_high},
    {0xf4, 0xf4, 4, continuation_low, 0x8f},
}};

// U+FFFD, written in place of a byte that is no part of a character.
inline constexpr std::string_view replacement_character = "\xef\xbf\xbd";

constexpr bool between(unsigned char c, unsigned char low, unsigned char high) {
  return c >= low && c <= high;
}

constexpr bool is_continuation(char c) {
  return between(byte(c), continuation_low, continuation_high);
}

// The rule for a first byte, or nothing when no character starts with it.
constexpr const utf8_lead* lead_of(unsigned char c) {
  for (const auto& lead : utf8_leads)
    if (between(c, lead.first, lead.last)) return &lead;
  return nullptr;
}

// The length of the character of more than one byte at text[i], or 0 when the
// bytes there are no character.
constexpr std::size_t sequence_length(std::string_view text, std::size_t i) {
  const utf8_lead* lead = lead_of(byte(text[i]));
  if (lead == nullptr || text.size() - i < lead->length) return 0;
  if (!between(byte(text[i + 1]), lead->second_low, lead->second_high)) return 0;
  for (std::size_t k = 2; k < lead->length; ++k)
    if (!is_continuation(text[i + k])) return 0;
  return lead->length;
}

// The length of the character at text[i], or 0 when the bytes there are no character.
constexpr std::size_t character_length(std::string_view text, std::size_t i) {
  return byte(text[i]) < first_non_ascii ? 1 : sequence_length(text, i);
}

// Whether `text` is well-formed UTF-8.
constexpr bool well_formed(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const auto length = character_length(text, i);
    if (length == 0) return false;
    i += length;
  }
  return true;
}

// At most the first `size` bytes of `text`, without the start of a character that
// the cut would split. It steps back over continuation bytes only as far as one
// character reaches.
constexpr std::string_view utf8_prefix(std::string_view text, std::size_t size) {
  if (text.size() <= size) return text;
  std::size_t end = size;
  for (std::size_t back = 1; back < longest_sequence && end > 0 && is_continuation(text[end]); ++back)
    --end;
  return text.substr(0, end);
}

// How many bytes from text[i] on go into a JSON string as they are: the length of a
// character that needs no escape, or 0.
constexpr std::size_t plain_length(std::string_view text, std::size_t i) {
  if (byte(text[i]) >= first_non_ascii) return sequence_length(text, i);
  return short_escape(text[i]).empty() && !is_control(text[i]) ? 1 : 0;
}

// Writes one byte of a string that cannot go as it is, the way JSON needs it.
template <sink S>
void write_escaped(S& out, char c) {
  if (const auto escape = short_escape(c); !escape.empty()) {
    std::invoke(out, escape);
  } else if (byte(c) >= first_non_ascii) {
    std::invoke(out, replacement_character);
  } else if (is_control(c)) {
    constexpr std::string_view digits = "0123456789abcdef";
    const auto u = static_cast<unsigned char>(c);
    const std::array<char, 6> text{'\\', 'u', '0', '0', digits[u >> 4], digits[u & 15]};
    std::invoke(out, std::string_view{text.data(), text.size()});
  }
}

}  // namespace detail

// Writes `text` as a JSON string: in quotes, with quotes, backslashes and control
// characters escaped. A well-formed UTF-8 character passes as it is; each byte that
// is no part of one becomes U+FFFD, so the result is always well-formed UTF-8.
template <sink S>
void write_string(S& out, std::string_view text) {
  std::invoke(out, std::string_view{"\""});
  std::size_t plain = 0;  // where the run of characters that need no escape starts
  for (std::size_t i = 0; i < text.size();) {
    if (const auto length = detail::plain_length(text, i); length != 0) {
      i += length;
      continue;
    }
    std::invoke(out, text.substr(plain, i - plain));
    detail::write_escaped(out, text[i]);
    plain = ++i;
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
