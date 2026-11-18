#pragma once

// Which LVGL setters take a list of options.
//
// lv_dropdown_set_options and lv_roller_set_options take a `const char*`, like every
// text. What makes it a list is LVGL's doc comment: "a string with '\n' separated
// options". So the comment is what this file reads. The headers of the widgets that
// have such a setter are embedded and searched once per file, only when a setter with a
// text is met. There is no list of functions here on purpose: it would be right for the
// LVGL it was written against.
//
// What can go wrong, and what catches it:
//  - LVGL adds a widget with a list of options. A setter named lv_<widget>_set_options
//    that is declared in a header not listed below is a compile error that names the
//    header.
//  - LVGL rewords the comment. Then no setter has the mark, which is a compile error.
//  - LVGL rewords one comment. tests/options.cpp sets and reads back a list through every
//    setter found.

#include <lv/detail/angle_docs.hpp>
#include <lv/detail/api.hpp>

#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

inline constexpr char dropdown_header_text[] = {
#embed <lvgl/widgets/lv_dropdown.h>
};
inline constexpr char roller_header_text[] = {
#embed <lvgl/widgets/lv_roller.h>
};

// Every LVGL header that declares a setter with a list of options.
// To add one: embed it above and list it here.
inline constexpr doc_header option_headers[] = {
    header("lvgl/widgets/lv_dropdown.h", dropdown_header_text),
    header("lvgl/widgets/lv_roller.h", roller_header_text),
};

// What a doc comment says about a parameter that takes a list of options.
inline constexpr std::string_view options_mark = "'\\n' separated options";

// A setter and the parameter that takes the list, as static strings.
struct option_parameter {
  const char* function;
  const char* parameter;
};

// The name that follows "@param " on the line of `at`, or an empty name.
consteval std::string_view parameter_on_line(std::string_view text, std::size_t at) {
  constexpr std::string_view tag = "@param ";
  std::size_t line = at;
  while (line > 0 && text[line - 1] != '\n') --line;
  for (std::size_t i = line; i + tag.size() < at; ++i) {
    if (text[i] != '@' || !text.substr(i).starts_with(tag)) continue;
    const std::size_t start = i + tag.size();
    std::size_t end = start;
    while (end < at && is_name_char(text[end])) ++end;
    return text.substr(start, end - start);
  }
  return {};
}

// Adds the setters of one header whose doc comment has the mark. One pass, which looks at
// about one character in ten (Horspool's search, with the table of lv/detail/angle_docs.hpp)
// through a plain pointer: what keeps the search cheap for the compiler.
consteval void collect_option_parameters(std::string_view text, std::vector<option_parameter>& out) {
  constexpr skips skip = skips_for(options_mark);
  constexpr std::size_t last = options_mark.size() - 1;
  constexpr char last_char = options_mark[last];
  const char* const chars = text.data();
  const std::size_t size = text.size();
  for (std::size_t at = 0; at + last < size; at += skip.by[static_cast<unsigned char>(chars[at + last])]) {
    if (chars[at + last] != last_char || !text.substr(at).starts_with(options_mark)) continue;
    const auto function = function_after(text, at);
    const auto parameter = parameter_on_line(text, at);
    if (function.empty() || parameter.empty()) continue;
    out.push_back({std::define_static_string(function), std::define_static_string(parameter)});
  }
}

// The setters of these headers whose doc comment has the mark.
consteval std::vector<option_parameter> options_in(std::span<const doc_header> headers) {
  std::vector<option_parameter> out;
  for (const doc_header& h : headers) collect_option_parameters(h.text, out);
  if (out.empty())
    fail("lv: no doc comment in LVGL's headers says '" + str(options_mark) +
         "' any more, so lvgl26 cannot tell which setters take a list of options. See "
         "lv/detail/option_docs.hpp: the mark it searches for must follow LVGL's wording.");
  return out;
}

template <class D>
consteval std::vector<option_parameter> read_option_headers() {
  return options_in(option_headers);
}

// The result of the one search of a file; a template so that nothing is searched until
// a setter with a text is met.
template <class D = void>
inline constexpr std::span<const option_parameter> option_parameters =
    std::define_static_array(read_option_headers<D>());

// Whether the parameter `parameter` of the setter `f` takes a list of options.
template <class D = void>
consteval bool takes_option_list(std::meta::info f, std::string_view parameter) {
  // The doc comments are read for a parameter called `options` only: the name keeps a label's
  // text, and every other text, from costing a look at the headers.
  if (parameter != "options") return false;
  const std::string_view name = std::meta::identifier_of(f);
  if (is_declared_in(f, option_headers)) {
    for (const auto& p : option_parameters<D>)
      if (name == p.function && parameter == p.parameter) return true;
    return false;
  }
  if (parameter == "options" && name.ends_with("_set_options"))
    fail("lv: '" + str(name) + "' takes options, and only LVGL's doc comment says whether they are "
         "a list separated by line breaks. lvgl26 has not read its header, " +
         str(std::meta::source_location_of(f).file_name()) +
         ". Add that header to option_headers in lv/detail/option_docs.hpp.");
  return false;
}

}  // namespace lv::detail
