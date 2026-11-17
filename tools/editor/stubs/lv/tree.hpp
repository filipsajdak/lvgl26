#pragma once

// Editor stub of lv/tree.hpp: a screen as a constexpr tree, and lv::mount.
//
//   constexpr auto ui = lv::column({.size = {320 * lv::px, 240 * lv::px}})[
//       lv::label({.id = "title", .text = "Speed"}),
//       lv::arc({.id = "speedo", .range = {0, 240}, .value = 120})];
//
//   auto root = lv::mount<ui>(display.screen());
//   root->get<"speedo">().set({.value = 0});

#include <lv/owned.hpp>
#include <lv/props.hpp>
#include <lv/view.hpp>

#include <cstddef>
#include <expected>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace lv {

template <detail::fixed W, class... Kids>
struct node;

namespace detail {

template <class T>
inline constexpr bool is_node = false;
template <fixed W, class... Kids>
inline constexpr bool is_node<node<W, Kids...>> = true;

template <class T>
concept tree_node = is_node<std::remove_cvref_t<T>>;

/// A node that builds no widget of its own, such as lv::each (lv/list.hpp).
template <class T>
inline constexpr bool is_structural = false;

/// What a node takes as a child: a node, or a structural node.
template <class T>
concept tree_child = tree_node<T> || is_structural<std::remove_cvref_t<T>>;

}  // namespace detail

/// One widget of a tree: its id and its children. (The real one also keeps the properties.)
template <detail::fixed W, class... Kids>
struct node {
  static constexpr auto kind = W;
  stub::id_name name{};
  std::tuple<Kids...> kids{};

  /// The children: lv::column()[lv::label(...), lv::arc(...)].
  template <detail::tree_child... K>
    requires(sizeof...(Kids) == 0)
  constexpr node<W, K...> operator[](const K&... k) const {
    return {name, {k...}};
  }

  /// The id of the node, or an empty text.
  constexpr std::string_view id() const { return name.sv(); }
};

namespace stub {

// The ids under a structural node, such as those of a row of lv::each, are not the tree's.
template <class S>
  requires detail::is_structural<S>
constexpr std::string_view kind_with_id(const S&, std::string_view) {
  return {};
}

// The kind of the node of a tree that has the id `id`: "arc". Empty when no node has it.
template <detail::fixed W, class... Kids>
constexpr std::string_view kind_with_id(const node<W, Kids...>& n, std::string_view id) {
  if (!id.empty() && n.id() == id) return W.sv();
  std::string_view found;
  std::apply([&](const auto&... kid) { ((found.empty() ? found = kind_with_id(kid, id) : found), ...); }, n.kids);
  return found;
}

template <const auto& Tree, detail::fixed Name>
inline constexpr std::string_view kind_of_id = kind_with_id(Tree, Name.sv());

/// The tree has a node with this id.
template <const auto& Tree, detail::fixed Name>
concept has_id = !kind_of_id<Tree, Name>.empty();

/// The view of the node with the id Name: lv::view<"arc"> for an arc.
template <const auto& Tree, detail::fixed Name>
using view_of_id =
    view<detail::fixed<kind_of_id<Tree, Name>.size() + 1>{kind_of_id<Tree, Name>.data(), kind_of_id<Tree, Name>.size()}>;

/// What lv::arc is: call it with properties, index it with children.
///   lv::arc({.value = 1})      lv::button({...})[lv::label({...})]      lv::box[...]
template <detail::fixed W>
struct factory {
  /// A node with these properties. They are in alphabetical order.
  constexpr node<W> operator()(const props<W>& p = {}) const { return {p.id}; }

  /// A node without properties, with these children.
  template <detail::tree_child... Kids>
  constexpr node<W, Kids...> operator[](const Kids&... kids) const {
    return {{}, {kids...}};
  }
};

}  // namespace stub

/// Any LVGL widget by name: lv::widget<"spinbox">({...}).
template <detail::fixed W>
  requires detail::known_widget<W>
inline constexpr stub::factory<W> widget{};

/// A plain box.
inline constexpr stub::factory<"obj"> box{};
/// A text.
inline constexpr stub::factory<"label"> label{};
/// A button. Its text is a label inside it.
inline constexpr stub::factory<"button"> button{};
/// A ring that shows or sets a value.
inline constexpr stub::factory<"arc"> arc{};
/// A bar that shows a value.
inline constexpr stub::factory<"bar"> bar{};
/// A bar with a knob to drag.
inline constexpr stub::factory<"slider"> slider{};
/// LVGL's switch: `switch` is a C++ keyword.
inline constexpr stub::factory<"switch"> toggle{};
/// A box to tick, with a text.
inline constexpr stub::factory<"checkbox"> checkbox{};
/// A picture: .src = lv::image_file<logo_png>.
inline constexpr stub::factory<"image"> image{};
/// A text to edit.
inline constexpr stub::factory<"textarea"> textarea{};
/// A list that drops down.
inline constexpr stub::factory<"dropdown"> dropdown{};
/// A wheel of choices.
inline constexpr stub::factory<"roller"> roller{};
/// A ring that turns while something is going on.
inline constexpr stub::factory<"spinner"> spinner{};
/// A box with none of the theme's styles that fills its parent. It draws nothing.
inline constexpr stub::factory<"obj"> group{};
/// A box that lays its children out from top to bottom.
inline constexpr stub::factory<"obj"> column{};
/// A box that lays its children out from left to right.
inline constexpr stub::factory<"obj"> row{};

/// Why lv::mount failed.
enum class mount_error {
  out_of_memory,  // LVGL could not create a widget
};

/// The widgets built from a tree. Owns them: they are deleted with this object.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
class mounted {
 public:
  mounted() = default;
  mounted(mounted&&) noexcept = default;
  mounted& operator=(mounted&&) noexcept = default;

  /// The root widget, or an empty view if its parent already deleted it.
  obj root() const { __builtin_unreachable(); }

  /// Gives the widgets to their parent: they now live until the parent is deleted.
  obj detach() { __builtin_unreachable(); }

  /// A typed view of the node with this id: get<"speedo">() is an lv::view<"arc">.
  template <detail::fixed Name>
    requires stub::has_id<Tree, Name>
  stub::view_of_id<Tree, Name> get() const { __builtin_unreachable(); }
};

/// Builds the widgets of the constexpr tree `Tree` under `parent`, and owns them.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
std::expected<mounted<Tree>, mount_error> mount(obj parent) { __builtin_unreachable(); }

/// Builds the widgets of `Tree` under `parent` and leaves them to it. Returns the
/// root, or an empty view if LVGL had no memory.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
obj place(obj parent) { __builtin_unreachable(); }

}  // namespace lv
