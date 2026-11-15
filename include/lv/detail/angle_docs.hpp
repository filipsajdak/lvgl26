#pragma once

// Which LVGL angles are in tenths of a degree.
//
// lv_arc_set_rotation takes whole degrees and lv_image_set_rotation takes tenths of
// a degree. Both are `int32_t`, and the names do not differ: the unit is written
// only in LVGL's doc comment. So the comment is what this file reads. The headers
// that declare a function named after an angle are embedded, and the functions whose
// comment says "0.1 degree" take and return an lv::fine_angle; every other angle is
// an lv::angle in whole degrees.
//
// There is no list of functions here on purpose. A list written by hand is right for
// the LVGL it was written against and silently wrong for the next one.
//
// What can go wrong, and what catches it:
//  - LVGL adds a header with an angle in it. A function named after an angle that is
//    declared in a header not listed below is a compile error that names the header.
//  - LVGL rewords every such comment. Then nothing has the mark, which is a compile
//    error too.
//  - LVGL rewords one comment, or a comment is wrong. Nothing here can see that. The
//    test tests/angle_units.cpp renders every angle property at v, v + 360 and
//    v + 3600 units: a property in degrees looks the same after 360, one in tenths
//    only after 3600. It fails when the pixels and the comment disagree.
//
// Cost, measured with /usr/bin/time: embedding the headers takes about 0.07 s in
// every file that includes this one. They are searched once per file, in one pass
// each (about 0.09 s for all of them), and only when the file uses a widget or a
// style that has an angle. Every later question is a lookup in a table of eight names.

#include <lv/detail/api.hpp>
#include <lv/units.hpp>

#include <cstdint>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

inline constexpr char style_header_text[] = {
#embed <lvgl/core/lv_obj_style_gen.h>
};
inline constexpr char arc_header_text[] = {
#embed <lvgl/widgets/lv_arc.h>
};
inline constexpr char arclabel_header_text[] = {
#embed <lvgl/widgets/lv_arclabel.h>
};
inline constexpr char image_header_text[] = {
#embed <lvgl/widgets/lv_image.h>
};
inline constexpr char scale_header_text[] = {
#embed <lvgl/widgets/lv_scale.h>
};
inline constexpr char spinner_header_text[] = {
#embed <lvgl/widgets/lv_spinner.h>
};

// A header of LVGL: its name as it is included, and its text.
struct doc_header {
  std::string_view path;
  std::string_view text;
};

template <std::size_t N>
constexpr doc_header header(std::string_view path, const char (&text)[N]) {
  return {path, std::string_view{text, N}};
}

// Every LVGL header that declares a setter or getter named after an angle.
// To add one: embed it above and list it here.
inline constexpr doc_header angle_headers[] = {
    header("lvgl/core/lv_obj_style_gen.h", style_header_text),
    header("lvgl/widgets/lv_arc.h", arc_header_text),
    header("lvgl/widgets/lv_arclabel.h", arclabel_header_text),
    header("lvgl/widgets/lv_image.h", image_header_text),
    header("lvgl/widgets/lv_scale.h", scale_header_text),
    header("lvgl/widgets/lv_spinner.h", spinner_header_text),
};

// What a doc comment says when its value is in tenths of a degree.
inline constexpr std::string_view tenths_mark = "0.1 degree";

// The searches below are written out, without std::string_view::find: with
// -fsanitize=undefined, GCC 16 does not treat the null check inside find as a
// constant when the view points into a static array.

// Where `c` is in `text` at or after `from`, or the size of the text.
consteval std::size_t position_of(std::string_view text, char c, std::size_t from) {
  while (from < text.size() && text[from] != c) ++from;
  return from;
}

// Where the comment that `at` is in ends: at its "*/". The size of the text if it
// does not end.
consteval std::size_t comment_end(std::string_view text, std::size_t at) {
  for (at = position_of(text, '*', at); at < text.size(); at = position_of(text, '*', at + 1))
    if (text.substr(at).starts_with("*/")) return at;
  return text.size();
}

// The name of the function declared after the comment that `at` is in, or an empty
// name if no declaration follows.
consteval std::string_view function_after(std::string_view text, std::size_t at) {
  const auto end = comment_end(text, at);
  const auto paren = position_of(text, '(', end);
  if (paren == text.size()) return {};
  // The name ends at the parenthesis and starts after the last character that is
  // not part of a name: the "*/" of the comment, if nothing else.
  auto start = paren;
  while (is_name_char(text[start - 1])) --start;
  return text.substr(start, paren - start);
}

// For Horspool's search: how far the search may move on when the text has the
// character `c` under the last character of the mark. A plain array in a struct:
// the search reads it once per step, and a call of std::array's operator[] would
// cost more there than the step itself.
struct skips {
  std::uint8_t by[256];
};
consteval skips skips_for(std::string_view mark) {
  skips skip{};
  for (auto& s : skip.by) s = static_cast<std::uint8_t>(mark.size());
  for (std::size_t i = 0; i + 1 < mark.size(); ++i)
    skip.by[static_cast<unsigned char>(mark[i])] = static_cast<std::uint8_t>(mark.size() - 1 - i);
  return skip;
}

// Whether the mark is in `text` at `at`, as a word of its own: "10.1 degrees" is
// not the mark.
consteval bool is_tenths_mark(std::string_view text, std::size_t at) {
  return text.substr(at).starts_with(tenths_mark) && (at == 0 || !is_name_char(text[at - 1]));
}

// Adds the functions of one header whose doc comment has the mark. One pass, which
// looks at about one character in five (Horspool's search) through a plain pointer:
// that is what keeps the search of LVGL's 140 KB style header cheap for the compiler.
consteval void collect_tenths(std::string_view text, std::vector<std::string_view>& out) {
  constexpr skips skip = skips_for(tenths_mark);
  constexpr std::size_t last = tenths_mark.size() - 1;
  constexpr char last_char = tenths_mark[last];
  const char* const chars = text.data();
  const std::size_t size = text.size();
  for (std::size_t at = 0; at + last < size;
       at += skip.by[static_cast<unsigned char>(chars[at + last])]) {
    if (chars[at + last] != last_char || !is_tenths_mark(text, at)) continue;
    if (const auto name = function_after(text, at); !name.empty()) out.push_back(name);
  }
}

// The functions of these headers whose doc comment says tenths of a degree.
consteval std::vector<const char*> functions_in_tenths(std::span<const doc_header> headers) {
  std::vector<std::string_view> names;
  for (const doc_header& h : headers) collect_tenths(h.text, names);
  if (names.empty())
    throw unknown_name("lv: no doc comment in LVGL's headers says '" + str(tenths_mark) +
                       "' any more, so lvgl26 cannot tell which angles are in tenths of a "
                       "degree. See lv/detail/angle_docs.hpp: the mark it searches for must "
                       "follow LVGL's wording.");
  std::vector<const char*> out;
  for (const auto name : names) out.push_back(std::define_static_string(name));
  return out;
}

// The result of the one search of a file. These two are templates only so that
// nothing is searched until a property with an angle is used: the compiler evaluates
// an initializer that does not depend on a template parameter where it is written.
template <class D>
consteval std::vector<const char*> read_angle_headers() {
  return functions_in_tenths(angle_headers);
}

template <class D = void>
inline constexpr std::span<const char* const> tenths_functions =
    std::define_static_array(read_angle_headers<D>());

// Whether `f` is declared in one of the headers that were read.
consteval bool is_declared_in(std::meta::info f, std::span<const doc_header> headers) {
  const std::string_view file = std::meta::source_location_of(f).file_name();
  for (const doc_header& h : headers)
    if (file.ends_with(h.path)) return true;
  return false;
}

// The type of an angle that the setter or getter `f` takes or returns: lv::fine_angle
// when its doc comment says tenths of a degree, lv::angle otherwise.
template <class D = void>
consteval std::meta::info angle_type_of(std::meta::info f) {
  const std::string_view name = std::meta::identifier_of(f);
  if (!is_declared_in(f, angle_headers))
    throw unknown_name(
        "lv: '" + str(name) + "' takes or returns an angle, and only LVGL's doc comment says "
        "whether it is in degrees or in tenths of a degree. lvgl26 has not read its header, " +
        str(std::meta::source_location_of(f).file_name()) +
        ". Add that header to angle_headers in lv/detail/angle_docs.hpp.");
  for (const char* in_tenths : tenths_functions<D>)
    if (name == in_tenths) return ^^fine_angle;
  return ^^angle;
}

}  // namespace lv::detail
