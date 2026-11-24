#pragma once

// Editor stub of lv/tokens.hpp: design tokens from a W3C Design Tokens (DTCG) file.
//
//   constexpr char tokens_json[] = {
//   #embed "tokens.json"
//       , 0};
//   constexpr auto tok = lv::tokens<tokens_json>;
//
// The struct of each token file of the project is in lv/token_files.hpp, under a key.
// The editor embeds a marker file in place of the token file (see overlay.yaml next to
// the stubs): it holds the key. Without the marker the key is worked out from the bytes.

#include <lv/config.hpp>
#include <lv/token_files.hpp>
#include <lv/token_types.hpp>
#include <lv/values.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace lv {

namespace stub {

inline constexpr std::string_view token_mark = "@lvgl26-editor-tokens:";

// The number after the mark.
constexpr std::uint64_t marked_key(std::string_view text) {
  std::uint64_t key = 0;
  for (char c : text.substr(token_mark.size())) {
    if (c < '0' || c > '9') break;
    key = key * 10 + static_cast<std::uint64_t>(c - '0');
  }
  return key;
}

constexpr std::uint64_t fnv(std::uint64_t key, std::uint64_t byte) {
  return (key ^ (byte & 0xff)) * 0x100000001b3ULL;
}

// The same number as key_of in tools/editor/token_stubs.py: FNV-1a over the size and every
// byte, the zeros at the end left out. The editor embeds a marker in place of a token file, so
// this runs on a file that the editor did not replace only.
constexpr std::uint64_t key_of_bytes(std::string_view text) {
  while (!text.empty() && text.back() == '\0') text.remove_suffix(1);
  std::uint64_t key = 0xcbf29ce484222325ULL;
  for (int i = 0; i < 8; ++i) key = fnv(key, static_cast<std::uint64_t>(text.size()) >> (8 * i));
  for (const char c : text) key = fnv(key, static_cast<unsigned char>(c));
  return key;
}

template <std::size_t N>
constexpr std::uint64_t token_key(const char (&doc)[N]) {
  const std::string_view text{doc, N};
  return text.starts_with(token_mark) ? marked_key(text) : key_of_bytes(text);
}

}  // namespace stub

namespace detail {
/// A document embedded as an array of char: `constexpr char doc[] = { #embed ... , 0 };`
template <class T>
concept embedded_text = std::is_bounded_array_v<std::remove_cvref_t<T>> &&
                        std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_cvref_t<T>>>, char>;
}  // namespace detail

/// The design tokens of the embedded DTCG document `Doc`, as a typed constant: a
/// struct for each group, an lv::token<T> for each token. A value in a form from before the Design Tokens
/// Format 2025.10 ("#3366ff", "8px") is an error in a build, unless the settings `O` allow it.
/// The editor does not read the setting.
template <const auto& Doc, class O = override>
  requires detail::embedded_text<decltype(Doc)>
inline constexpr typename stub::token_file<stub::token_key(Doc)>::type tokens{};

}  // namespace lv
