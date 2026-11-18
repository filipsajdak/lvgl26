// The setters that lvgl26 takes a list of options for, found by name (lv/detail/option_docs.hpp),
// must be those that LVGL's doc comments call "'\n' separated options". The comments are read
// here, from the headers of the widgets, and not by the library: see the header for the cost.

#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>
#include <string_view>
#include <vector>

namespace {

constexpr char dropdown_header[] = {
#embed <lvgl/widgets/lv_dropdown.h>
};
constexpr char roller_header[] = {
#embed <lvgl/widgets/lv_roller.h>
};

constexpr std::string_view mark = "'\\n' separated options";

// The setters of a header whose doc comment has the mark.
consteval std::vector<std::string_view> marked_in(std::string_view text) {
  std::vector<std::string_view> out;
  for (std::size_t at = 0; at + mark.size() <= text.size(); ++at)
    if (text[at] == mark[0] && text.substr(at).starts_with(mark))
      if (const auto function = lv::detail::function_after(text, at); !function.empty()) out.push_back(function);
  return out;
}

consteval bool is_marked(std::string_view name) {
  for (const auto text : {std::string_view{dropdown_header, sizeof dropdown_header},
                          std::string_view{roller_header, sizeof roller_header}})
    for (const auto function : marked_in(text))
      if (function == name) return true;
  return false;
}

// What the comments mark, and nothing else, is what the names find.
static_assert(is_marked("lv_dropdown_set_options") && is_marked("lv_dropdown_set_options_static") &&
              is_marked("lv_roller_set_options"));
static_assert(!is_marked("lv_dropdown_set_text") && !is_marked("lv_dropdown_set_selected"));

static_assert(lv::detail::takes_option_list(^^lv_dropdown_set_options, "options"));
static_assert(lv::detail::takes_option_list(^^lv_roller_set_options, "options"));
static_assert(!lv::detail::takes_option_list(^^lv_dropdown_set_text, "txt"));
static_assert(!lv::detail::takes_option_list(^^lv_label_set_text, "text"));

// Every setter with the name has the mark in its comment, and the other way round, for the
// setters of these two headers.
consteval bool names_and_comments_agree() {
  for (auto f : lv::detail::api) {
    const auto name = std::meta::identifier_of(f);
    if (!name.starts_with("lv_dropdown_set_") && !name.starts_with("lv_roller_set_")) continue;
    const auto params = std::meta::parameters_of(f);
    if (params.size() < 2 || !std::meta::has_identifier(params[1])) continue;
    const bool by_name = lv::detail::takes_option_list(f, std::meta::identifier_of(params[1]));
    // lv_dropdown_set_options_static is marked too, and is what a list is passed as: no list for it.
    if (by_name != (is_marked(name) && !name.ends_with("_static"))) return false;
  }
  return true;
}
static_assert(names_and_comments_agree());

}  // namespace

int main() {
  return failures == 0 ? 0 : 1;
}
