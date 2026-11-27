#pragma once

// Icons as text: the names of Phosphor's icons (MIT), each a character of an icon font. The name is
// checked where it is written, and a wrong one lists the nearest names.
//
//   lv::glyph<"arrow-left">                     the text of one icon: what a label shows
//   lv::glyphs<"arrow-left", "check", "gear">   the characters a font of those icons holds
//
// The font draws only the glyphs it is asked for: name them once, in the font's typeface. The file of
// the font is in lv/icons_font.hpp, so that a file that only names icons does not embed it.

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/nearest.hpp>
#include <lv/tree.hpp>

#include <cstddef>
#include <cstdint>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

// The icons of Phosphor: a line for each, sorted by name: "arrow-left e058".
// Written by tools/icons_names.py from Phosphor's style sheet.
constexpr char phosphor_names_text[] = {
#embed "icons/phosphor.names"
    , 0};

// The names text as a view: its length is known, so no loop counts it (a view made from a const char* alone does).
consteval std::string_view phosphor_names() { return {phosphor_names_text, sizeof phosphor_names_text - 1}; }

// One line of the names text: the name, then the code point in hexadecimal.
struct icon_entry {
  std::string_view name;
  std::uint32_t code = 0;
};

// The line that starts at `at`, and where the next one starts.
consteval icon_entry icon_line(std::string_view text, std::size_t& at) {
  const std::size_t space = text.find(' ', at);
  const std::size_t end = text.find('\n', space);
  icon_entry entry{text.substr(at, space - at), 0};
  for (std::size_t i = space + 1; i < end; ++i)
    entry.code = entry.code * 16 + static_cast<std::uint32_t>(text[i] <= '9' ? text[i] - '0' : text[i] - 'a' + 10);
  at = end + 1;
  return entry;
}

// The code point of the icon `name`, or 0. The lines are sorted by name, so the search halves the text and
// steps back to the start of a line: 11 lines are read, not 1530.
consteval std::uint32_t code_of_icon(std::string_view name) {
  const std::string_view text = phosphor_names();
  std::size_t low = 0, high = text.size();
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2;
    const std::size_t found = text.rfind('\n', middle);
    std::size_t start = found == std::string_view::npos ? 0 : found + 1;
    if (start < low) start = low;
    std::size_t next = start;
    const icon_entry entry = icon_line(text, next);
    if (entry.name == name) return entry.code;
    if (entry.name < name)
      low = next;
    else
      high = start;
  }
  return 0;
}

// The icons nearest to `wanted`, nearest first, at most `names_offered`: one pass over the names, asking
// the distance only of names about as long as the wanted one (a typo keeps the length), so that the
// message of a wrong name stays far under the limit of a constant evaluation.
consteval std::vector<std::string_view> nearest_icons(std::string_view wanted) {
  const std::string_view text = phosphor_names();
  std::vector<std::string_view> best;
  std::vector<std::size_t> far;
  for (std::size_t at = 0; at < text.size();) {
    const icon_entry entry = icon_line(text, at);
    const std::size_t length = entry.name.size();
    if (length + 3 < wanted.size() || wanted.size() + 3 < length) continue;
    const std::size_t d = distance_between(wanted, entry.name);
    std::size_t place = best.size();
    while (place > 0 && far[place - 1] > d) --place;
    if (place >= names_offered) continue;
    best.insert(best.begin() + static_cast<std::ptrdiff_t>(place), entry.name);
    far.insert(far.begin() + static_cast<std::ptrdiff_t>(place), d);
    if (best.size() > names_offered) best.pop_back(), far.pop_back();
  }
  return best;
}

// The UTF-8 bytes of a character in the private use area, as the font draws it.
consteval std::string utf8_of(std::uint32_t code) {
  std::string out;
  if (code < 0x800) {
    out += static_cast<char>(0xc0 | code >> 6);
    out += static_cast<char>(0x80 | (code & 0x3f));
  } else {
    out += static_cast<char>(0xe0 | code >> 12);
    out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    out += static_cast<char>(0x80 | (code & 0x3f));
  }
  return out;
}

// The text of the icon `name`; a name that is none stops the build and lists the nearest.
consteval std::string glyph_text(std::string_view name) {
  const std::uint32_t code = code_of_icon(name);
  if (code == 0) {
    const auto near = nearest_icons(name);
    std::string list;
    for (const auto n : near) list += (list.empty() ? "" : ", ") + str(n);
    const std::string maybe = !near.empty() && distance_between(name, near.front()) <= close_enough
                                  ? " Did you mean '" + str(near.front()) + "'?"
                                  : std::string{};
    fail("lv: there is no icon '" + str(name) + "' in Phosphor." + maybe + " The nearest names: " + list + ".");
  }
  return utf8_of(code);
}

consteval std::string glyphs_text(std::initializer_list<std::string_view> names) {
  std::string out;
  for (const std::string_view name : names) out += glyph_text(name);
  return out;
}

}  // namespace lv::detail

namespace lv {

// The text of one icon, for `.text` of a label.
template <detail::fixed Name>
inline constexpr const char* glyph = std::define_static_string(detail::glyph_text(Name.sv()));

// The characters of several icons, for the `characters` of a typeface file: the font draws only those.
template <detail::fixed... Names>
inline constexpr std::string_view glyphs = std::define_static_string(detail::glyphs_text({Names.sv()...}));

// A label that shows the icon `Name`, with the properties of a label: lv::icon<"gear">({.style = ...}).
// Its font and colour come from the style, as for any label.
template <detail::fixed Name, class D = void>
constexpr auto icon(const detail::lazy_props<"label", D>& p = {}) {
  auto own = p;
  own.text = glyph<Name>;
  return label(own);
}


}  // namespace lv
