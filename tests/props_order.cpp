// The members of a props struct are in the order a tree is written in: the id, the properties, the styles,
// the handlers, each group alphabetical (include/lv/props.hpp). A tree written in another order is a compile
// error that names one field, so this order is what users see.

#include <lv/lv.hpp>

#include <meta>
#include <string_view>

namespace {

using namespace std::string_view_literals;

consteval int group_of(std::string_view name) {
  if (name == "id"sv) return 0;
  if (name.starts_with("style"sv)) return 2;
  if (name.starts_with("on_"sv)) return 3;
  return 1;
}

// True when every member comes after the one before it: by group, then by name.
template <lv::detail::fixed W>
consteval bool in_the_order_of_a_tree() {
  std::string_view before;
  int group_before = -1;
  bool first = true;
  for (auto m : std::meta::nonstatic_data_members_of(^^lv::props<W>, std::meta::access_context::unchecked())) {
    const std::string_view name = std::meta::identifier_of(m);
    if (name == "made_with_braces"sv) continue;  // the mark at the end of every props struct
    const int group = group_of(name);
    if (first) {
      if (name != "id"sv) return false;
      first = false;
    } else if (group < group_before || (group == group_before && !(before < name))) {
      return false;
    }
    before = name;
    group_before = group;
  }
  return !first;
}

static_assert(in_the_order_of_a_tree<"obj">() && in_the_order_of_a_tree<"label">() && in_the_order_of_a_tree<"button">());
static_assert(in_the_order_of_a_tree<"arc">() && in_the_order_of_a_tree<"slider">() && in_the_order_of_a_tree<"dropdown">());
static_assert(in_the_order_of_a_tree<"roller">() && in_the_order_of_a_tree<"textarea">() && in_the_order_of_a_tree<"buttonmatrix">());
static_assert(in_the_order_of_a_tree<"table">() && in_the_order_of_a_tree<"checkbox">() && in_the_order_of_a_tree<"image">());

// Where a group begins and ends in a widget that has all four.
template <lv::detail::fixed W>
consteval bool has_every_group() {
  bool seen[4] = {false, false, false, false};
  for (auto m : std::meta::nonstatic_data_members_of(^^lv::props<W>, std::meta::access_context::unchecked()))
    seen[group_of(std::meta::identifier_of(m))] = true;
  return seen[0] && seen[1] && seen[2] && seen[3];
}
static_assert(has_every_group<"button">() && has_every_group<"label">());

}  // namespace

int main() {}
