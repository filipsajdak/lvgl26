// The call form of a factory, lv::group({props}, child, child), builds the node that the subscript
// form lv::group({props})[child, child] builds. The structure is the type of the node, so the types are
// compared; the widgets that a mounted tree has are counted.

#include <lv/lv.hpp>
#include <lv/repeat.hpp>

#include "check.hpp"

#include <concepts>

namespace {

using namespace lv::units;

enum class step { first, second, third };

constexpr auto label_a = lv::label({.id = "a", .text = "A"});
constexpr auto label_b = lv::label({.id = "b", .text = "B"});
constexpr auto tile = []<step S> { return lv::label({.text = lv::name_of<S>}); };

// With properties and children, with children alone, with properties alone, with nothing.
constexpr auto with_both = lv::group({.id = "g"}, label_a, label_b);
static_assert(std::same_as<std::remove_const_t<decltype(with_both)>,
                           decltype(lv::group({.id = "g"})[label_a, label_b])>);
constexpr auto kids_alone = lv::column(label_a, label_b);
static_assert(std::same_as<std::remove_const_t<decltype(kids_alone)>, decltype(lv::column[label_a, label_b])>);
constexpr auto props_alone = lv::group({.id = "g"});

// A column starts as a column, with or without children: the kind of start is kept.
constexpr auto column_start = lv::column().how;
constexpr auto row_start = lv::row().how;
static_assert(column_start != row_start && column_start == lv::column(label_a).how && column_start == lv::column[label_a].how);
static_assert(row_start == lv::row({.id = "r"}, label_a).how && lv::group().how == lv::group({.id = "g"}, label_a).how);

// A repeated child is spliced in among the others, in the order they are written.
constexpr auto repeated = lv::column({.id = "r"}, label_a, lv::for_each<step>(tile), label_b);
static_assert(std::same_as<std::remove_const_t<decltype(repeated)>,
                           decltype(lv::column({.id = "r"})[label_a, lv::for_each<step>(tile), label_b])>);
constexpr auto repeated_alone = lv::column(lv::for_each<step>(tile));
static_assert(std::same_as<std::remove_const_t<decltype(repeated_alone)>, decltype(lv::column[lv::for_each<step>(tile)])>);

constinit lv::frame<160, 120> frame;

void children_are_built_in_order(lv::display& display) {
  auto both = lv::mount<with_both>(display.screen());
  check(both.has_value());
  if (!both) return;
  lv_obj_t* group = both->get<"g">().raw();
  check(lv_obj_get_child_count(group) == 2);
  check(both->get<"a">().raw() == lv_obj_get_child(group, 0) && both->get<"b">().raw() == lv_obj_get_child(group, 1));
}

void a_repeated_child_adds_its_widgets_in_place(lv::display& display) {
  auto built = lv::mount<repeated>(display.screen());
  check(built.has_value());
  if (!built) return;
  lv_obj_t* column = built->get<"r">().raw();
  check(lv_obj_get_child_count(column) == 5);  // A, three steps, B
  check(built->get<"a">().raw() == lv_obj_get_child(column, 0) && built->get<"b">().raw() == lv_obj_get_child(column, 4));
}

void a_node_without_children_has_none(lv::display& display) {
  auto built = lv::mount<props_alone>(display.screen());
  check(built.has_value());
  if (!built) return;
  check(lv_obj_get_child_count(built->get<"g">().raw()) == 0);
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  children_are_built_in_order(display);
  a_repeated_child_adds_its_widgets_in_place(display);
  a_node_without_children_has_none(display);
  return failures == 0 ? 0 : 1;
}
