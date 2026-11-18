#pragma once

// Which LVGL setters take a list of options.
//
// lv_dropdown_set_options and lv_roller_set_options take a `const char*`, like every
// text. They are told from a text by their name: a setter that ends in `_set_options` and
// takes a text called `options`. (lv_dropdown_set_options_static stays a plain text: it is what
// the list uses, in the library, and nobody needs a list for it.) LVGL documents them as "a string
// with '\n' separated options"; tests/options_docs.cpp reads that comment from the headers
// and fails when the names and the comments disagree. The reading is in a test, not here,
// because a header that embeds and searches LVGL's headers is paid for by every file that
// includes lvgl26, measured at 0.2% of the compile time of tests/large_tree.cpp.
//
// A list is only another way to write the one text that LVGL takes, so a setter that takes
// the name without the meaning gets a list that is joined and passed as the text it would
// have been given: nothing breaks.

#include <meta>
#include <string_view>

namespace lv::detail {

// Whether the setter `f`, which takes the text called `parameter`, takes it as a list.
consteval bool takes_option_list(std::meta::info f, std::string_view parameter) {
  const std::string_view name = std::meta::identifier_of(f);
  return parameter == "options" && name.ends_with("_set_options");
}

}  // namespace lv::detail
