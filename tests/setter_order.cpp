// The setters of a widget are called in the order of its props, unless include/lv/detail/setter_order.hpp
// says otherwise. Each rule of that table has a test here that reads what LVGL did, for a tree
// and for view.set(). The members are written in the order of the props: the order of the calls
// is what differs.

#include <lv/lv.hpp>

#include "check.hpp"

namespace {

constinit lv::frame<64, 48> frame;

constexpr const char* const keys[] = {"A", "B", "C", nullptr};
// The second button is checked, and every button can be.
constexpr lv_buttonmatrix_ctrl_t controls[] = {LV_BUTTONMATRIX_CTRL_CHECKABLE,
                                               static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKABLE | LV_BUTTONMATRIX_CTRL_CHECKED),
                                               LV_BUTTONMATRIX_CTRL_CHECKABLE};

// Every rule names a widget and a property that exist, and the table is the one the tests below cover.
template <class Props>
consteval bool has_member(std::string_view name) {
  for (auto m : std::meta::nonstatic_data_members_of(^^Props, std::meta::access_context::unchecked()))
    if (std::meta::identifier_of(m) == name) return true;
  return false;
}
static_assert(has_member<lv::props<"buttonmatrix">>("map") && has_member<lv::props<"buttonmatrix">>("ctrl_map"));
static_assert(has_member<lv::props<"textarea">>("cursor_pos"));
// A new rule needs a test in this file, and its widget a line here.
static_assert(lv::detail::has_setter_rule<"buttonmatrix"> && lv::detail::has_setter_rule<"textarea">);
static_assert(!lv::detail::has_setter_rule<"label"> && !lv::detail::has_setter_rule<"roller">);
static_assert(lv::detail::setter_phase_of<"buttonmatrix">("map") == lv::detail::setter_first);
static_assert(lv::detail::setter_phase_of<"buttonmatrix">("ctrl_map") == lv::detail::setter_last);
static_assert(lv::detail::setter_phase_of<"textarea">("cursor_pos") == lv::detail::setter_last);
static_assert(lv::detail::setter_phase_of<"buttonmatrix">("one_checked") == lv::detail::setter_normal);
static_assert(lv::detail::setter_phase_of<"label">("map") == lv::detail::setter_normal);

constexpr auto matrix = lv::widget<"buttonmatrix">({.id = "keys", .ctrl_map = controls, .map = keys, .one_checked = true});
constexpr auto area = lv::textarea({.id = "area", .cursor_pos = 2, .one_line = true, .text = "Block"});
constexpr auto empty_matrix = lv::widget<"buttonmatrix">({.id = "keys"});
constexpr auto empty_area = lv::textarea({.id = "area", .one_line = true});

bool checked(lv_obj_t* obj, std::uint32_t button) {
  return lv_buttonmatrix_has_button_ctrl(obj, button, LV_BUTTONMATRIX_CTRL_CHECKED);
}

// A new map clears the controls, and set_one_checked checks the first button again: so the map
// comes first and ctrl_map last, whatever the order of the names.
void a_tree_sets_the_map_before_the_controls(lv::display& display) {
  auto root = lv::mount<matrix>(display.screen());
  check(root.has_value());
  if (!root) return;
  lv_obj_t* obj = root->get<"keys">().raw();
  check(!checked(obj, 0));
  check(checked(obj, 1));
  check(!checked(obj, 2));
}

void a_set_does_the_same(lv::display& display) {
  auto root = lv::mount<empty_matrix>(display.screen());
  if (!root) return;
  root->get<"keys">().set({.ctrl_map = controls, .map = keys, .one_checked = true});
  lv_obj_t* obj = root->get<"keys">().raw();
  check(!checked(obj, 0));
  check(checked(obj, 1));
  check(!checked(obj, 2));
}

// set_text moves the cursor to the end, so the cursor position is set after the text.
void a_tree_sets_the_cursor_after_the_text(lv::display& display) {
  auto root = lv::mount<area>(display.screen());
  check(root.has_value());
  if (!root) return;
  check(lv_textarea_get_cursor_pos(root->get<"area">().raw()) == 2);
}

void a_set_does_the_same_for_the_cursor(lv::display& display) {
  auto root = lv::mount<empty_area>(display.screen());
  if (!root) return;
  // The position is a value the compiler cannot fold.
  const volatile int position = 3;
  root->get<"area">().set({.cursor_pos = position, .text = "Notes!"});
  check(lv_textarea_get_cursor_pos(root->get<"area">().raw()) == 3);
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  a_tree_sets_the_map_before_the_controls(display);
  a_set_does_the_same(display);
  a_tree_sets_the_cursor_after_the_text(display);
  a_set_does_the_same_for_the_cursor(display);
  return failures == 0 ? 0 : 1;
}
