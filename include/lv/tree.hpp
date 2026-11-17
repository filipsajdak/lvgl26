#pragma once

// L2: a UI described as a constexpr value, and lv::mount to build it.
//
//   constexpr auto ui = lv::column({.size = {320 * lv::px, 240 * lv::px}})[
//       lv::label({.id = "title", .text = "Speed"}),
//       lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
//       lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];
//
//   auto root = lv::mount<ui>(lv::screen_active());
//   root->get<"speedo">().set({.value = 0});
//
// Properties go in (), in alphabetical order; children go in []. A node without
// properties needs no (): lv::column[lv::label(...)].
// The tree is read at compile time. Only the calls for the properties that are
// set end up in the program; the tree itself is not stored in it.

#include <lv/owned.hpp>
#include <lv/props.hpp>
#include <lv/view.hpp>

#include <array>
#include <expected>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

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

template <class... Kids>
struct kids_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, numbered_members('k', {^^Kids...})); }
};

// The children of a node: a struct with the members k0, k1, ..., one for each child.
// Reflection makes it, so that all children are written into it in one step. A list
// built child by child copied the children after each one again, and the compiler
// walks every member of a node it copies.
template <class... Kids>
using kids = typename kids_of<Kids...>::type;

template <class List>
inline constexpr auto kid_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^List, std::meta::access_context::unchecked()));

template <std::size_t I, class List>
constexpr const auto& kid(const List& l) {
  return l.[:kid_members<List>[I]:];
}

}  // namespace detail

// One widget of a tree: its properties and its children.
template <detail::fixed W, class... Kids>
struct node {
  static constexpr auto kind = W;
  detail::kept<W> p{};  // what props<W> gave it: see detail::kept_of
  detail::kids<Kids...> kids{};
  bool bare = false;  // start without the theme's styles: see lv::group

  // The children: lv::column()[lv::label(...), lv::arc(...)].
  template <detail::tree_node... K>
    requires(sizeof...(Kids) == 0)
  constexpr node<W, K...> operator[](const K&... k) const {
    return {p, {k...}, bare};
  }

  constexpr std::string_view id() const {
    return p.id.set ? std::string_view{p.id.v.v0} : std::string_view{};
  }
};

namespace detail {

// The functions below must not name node<"arc"> directly: that would generate
// the property struct of every listed widget in every file that includes this.
template <fixed W, class D>
constexpr node<W> make_node(const lazy_props<W, D>& p) {
  return {keep<W, D>(p)};
}

template <class D>
constexpr auto make_flex(const lazy_props<"obj", D>& p, lv_flex_flow_t flow) {
  auto n = make_node<"obj", D>(p);
  if (!n.p.flex_flow.set) n.p.flex_flow = flow;
  return n;
}

}  // namespace detail

namespace detail {

// What lv::arc is: call it with properties, index it with children.
//   lv::arc({.value = 1})      lv::button({...})[lv::label({...})]      lv::box[...]
template <fixed W>
struct factory {
  template <class D = void>
  constexpr auto operator()(const lazy_props<W, D>& p = {}) const {
    return make_node<W, D>(p);
  }

  template <tree_node... Kids>
  constexpr auto operator[](const Kids&... kids) const {
    return (*this)()[kids...];
  }
};

// lv::group: a box with none of the theme's styles (no background, border, padding
// or scrollbars) that fills its parent, unless .size says otherwise. It groups
// widgets without drawing anything itself.
struct group_factory {
  template <class D = void>
  constexpr auto operator()(const lazy_props<"obj", D>& p = {}) const {
    auto n = make_node<"obj", D>(p);
    if (!n.p.size.set) n.p.size = {percent{100}, percent{100}};
    n.bare = true;
    return n;
  }

  // The call names a type that depends on Kids. A plain (*this)() does not depend on
  // anything, so the compiler would work it out, and generate props<"obj">, while it
  // reads this header.
  template <tree_node... Kids>
  constexpr auto operator[](const Kids&... kids) const {
    return this->operator()<std::void_t<Kids...>>()[kids...];
  }
};

// lv::column and lv::row: a box laid out as a flex container, unless .flex_flow
// says otherwise.
struct flex_factory {
  lv_flex_flow_t flow;

  template <class D = void>
  constexpr auto operator()(const lazy_props<"obj", D>& p = {}) const {
    return make_flex<D>(p, flow);
  }

  template <tree_node... Kids>
  constexpr auto operator[](const Kids&... kids) const {
    return this->operator()<std::void_t<Kids...>>()[kids...];  // see group_factory
  }
};

}  // namespace detail

// Any LVGL widget by name: lv::widget<"spinbox">({...}).
template <detail::fixed W>
inline constexpr detail::factory<W> widget{};

// Short names for the common widgets. C++ cannot generate these names, so this
// list is written by hand; lv::widget<"..."> reaches every other widget.
inline constexpr detail::factory<"obj"> box{};
inline constexpr detail::factory<"label"> label{};
inline constexpr detail::factory<"button"> button{};
inline constexpr detail::factory<"arc"> arc{};
inline constexpr detail::factory<"bar"> bar{};
inline constexpr detail::factory<"slider"> slider{};
inline constexpr detail::factory<"switch"> toggle{};
inline constexpr detail::factory<"checkbox"> checkbox{};
inline constexpr detail::factory<"image"> image{};
inline constexpr detail::factory<"textarea"> textarea{};
inline constexpr detail::factory<"dropdown"> dropdown{};
inline constexpr detail::factory<"roller"> roller{};
inline constexpr detail::factory<"spinner"> spinner{};
inline constexpr detail::group_factory group{};
inline constexpr detail::flex_factory column{LV_FLEX_FLOW_COLUMN};
inline constexpr detail::flex_factory row{LV_FLEX_FLOW_ROW};

// ---- compile-time facts about a tree ----------------------------------------

namespace detail {

struct id_entry {
  std::string_view name;
  std::meta::info view;  // the view<...> type of the node
};

template <fixed W, class... Kids>
consteval void collect(std::vector<id_entry>& out, const node<W, Kids...>& n) {
  if (!n.id().empty()) out.push_back({n.id(), ^^view<W>});
  template for (constexpr auto k : kid_members<kids<Kids...>>) collect(out, n.kids.[:k:]);
}

// The ids of a tree, in the order mount meets them.
template <const auto& Tree>
consteval std::vector<id_entry> ids_of() {
  std::vector<id_entry> out;
  collect(out, Tree);
  for (std::size_t i = 0; i < out.size(); ++i)
    for (std::size_t j = i + 1; j < out.size(); ++j)
      if (out[i].name == out[j].name)
        fail("lv: the id '" + str(out[i].name) +
                           "' is used twice in one tree. Ids must be unique.");
  return out;
}

template <const auto& Tree>
inline constexpr std::size_t id_count = ids_of<Tree>().size();

template <const auto& Tree>
consteval std::size_t id_index(std::string_view name) {
  const auto ids = ids_of<Tree>();
  std::string valid;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    if (ids[i].name == name) return i;
    if (!valid.empty()) valid += ", ";
    valid += ids[i].name;
  }
  fail("lv: the tree has no id '" + str(name) + "'. Its ids: " +
                     (valid.empty() ? str("none") : valid) + ".");
}

template <const auto& Tree>
consteval std::meta::info id_view(std::string_view name) {
  return ids_of<Tree>()[id_index<Tree>(name)].view;
}

// ---- building ---------------------------------------------------------------

struct build_state {
  lv_obj_t** next_id;
  bool ok = true;
};

template <class T>
inline constexpr std::size_t kid_count = 0;
template <fixed W, class... Kids>
inline constexpr std::size_t kid_count<node<W, Kids...>> = sizeof...(Kids);

// The node of the tree `root` reached by taking child number Path[0], then child
// number Path[1] of that, and so on.
template <class Node>
constexpr const Node& descend(const Node& n) {
  return n;
}
template <std::size_t First, std::size_t... Rest, class Node>
constexpr const auto& descend(const Node& n) {
  return descend<Rest...>(kid<First>(n.kids));
}

// Names one style of one node of a tree, for add_const_style.
template <const auto& Tree, std::size_t Member, std::size_t... Path>
struct style_at {
  static constexpr const auto& get() {
    const auto& n = descend<Path...>(Tree);
    using node_type = std::remove_cvref_t<decltype(n)>;
    return n.p.[:members_of_kept<node_type::kind>[Member]:].value;
  }
};

// The numbers of the members of props<W> that the node `n` sets.
template <fixed W, class... Kids>
consteval std::vector<std::size_t> set_members_of(const node<W, Kids...>& n) {
  std::vector<std::size_t> out;
  template for (constexpr auto i : indices<members_of_kept<W>.size()>) {
    if (is_set(n.p.[:members_of_kept<W>[i]:])) out.push_back(i);
  }
  return out;
}

// The same for the node of `Tree` at `Path`. They are found in one constant
// evaluation, and apply_node visits only these: a question to the compiler for each
// of the 160 members of each node is what made a large tree slow to mount.
template <const auto& Tree, std::size_t... Path>
inline constexpr std::span<const std::size_t> set_members =
    std::define_static_array(set_members_of(descend<Path...>(Tree)));

// Applies the properties of one node of `Tree` to its widget `w`.
// A node is named by the root and an index path, never by a reference to the node
// itself: see the note above const_count in style.hpp.
template <const auto& Tree, std::size_t... Path>
void apply_node(lv_obj_t* w) {
  constexpr const auto& N = descend<Path...>(Tree);
  [[maybe_unused]] constexpr auto W = std::remove_cvref_t<decltype(N)>::kind;

  template for (constexpr auto i : set_members<Tree, Path...>) {
    constexpr auto m = members_of_kept<W>[i];
    if constexpr (is_maybe<std::remove_cvref_t<decltype(N.p.[:m:])>>) {
      add_const_style<style_at<Tree, i, Path...>>(w, selector_of<targets<W>[i]>());
    } else {
      constexpr auto a = N.p.[:m:];  // a constant: the tree itself is not kept
      static_assert(!a.follows || bindable<W, i>(),
                    "lv: this property cannot follow a state. LVGL has no bind function "
                    "for it and it takes more than one value.");
      apply_member<W, i, std::remove_cvref_t<decltype(a)>, true>(w, a);
    }
  }
}

// Builds one node of the constexpr tree `Tree` under `parent`, with its children.
template <const auto& Tree, std::size_t... Path>
lv_obj_t* build(lv_obj_t* parent, build_state& st) {
  constexpr const auto& N = descend<Path...>(Tree);
  using node_type = std::remove_cvref_t<decltype(N)>;

  auto w = view<node_type::kind>::create(obj{parent});
  if (!w) {
    st.ok = false;
    return nullptr;
  }
  if constexpr (!N.id().empty()) *st.next_id++ = w.raw();
  if constexpr (N.bare) lv_obj_remove_style_all(w.raw());
  apply_node<Tree, Path...>(w.raw());

  template for (constexpr auto i : indices<kid_count<node_type>>) {
    if (st.ok) build<Tree, Path..., i>(w.raw(), st);
  }
  return w.raw();
}

}  // namespace detail

// ---- the mounted tree -------------------------------------------------------

enum class mount_error {
  out_of_memory,  // LVGL could not create a widget
};

// The widgets built from a tree. Owns them: they are deleted with this object.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
class mounted {
  owned<obj> root_;
  std::array<lv_obj_t*, detail::id_count<Tree>> ids_{};

  template <const auto& T>
    requires detail::tree_node<decltype(T)>
  friend std::expected<mounted<T>, mount_error> mount(obj);

 public:
  mounted() = default;
  mounted(mounted&&) noexcept = default;
  mounted& operator=(mounted&&) noexcept = default;

  // The root widget, or an empty view if its parent already deleted it.
  obj root() const { return root_.get(); }

  // Gives the widgets to their parent: they now live until the parent is deleted,
  // as widgets made with LVGL's C API do. Returns the root.
  obj detach() { return root_.release(); }

  // A typed view of the node with this id.
  template <detail::fixed Name>
  auto get() const {
    // A check in the body, not pre(): GCC 16.2 crashes on pre() with a deduced return type.
    contract_assert(root_ && "the mounted tree was deleted by its parent");
    using view_type = typename[:detail::id_view<Tree>(Name.sv()):];
    return view_type{ids_[detail::id_index<Tree>(Name.sv())]};
  }
};

// Builds the widgets of the constexpr tree `Tree` under `parent`.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
std::expected<mounted<Tree>, mount_error> mount(obj parent)
  pre(parent.raw() != nullptr)
{
  mounted<Tree> m;
  detail::build_state st{m.ids_.data()};
  lv_obj_t* root = detail::build<Tree>(parent.raw(), st);
  m.root_ = owned<obj>{obj{root}};  // also deletes a half-built tree on failure
  if (!st.ok || !m.root_) return std::unexpected(mount_error::out_of_memory);
  return m;
}

// Builds the widgets of `Tree` under `parent` and leaves them to it, as widgets
// made with LVGL's C API are: they live until the parent is deleted.
// Returns the root, or an empty view if LVGL had no memory.
template <const auto& Tree>
  requires detail::tree_node<decltype(Tree)>
obj place(obj parent)
  pre(parent.raw() != nullptr)
{
  auto built = mount<Tree>(parent);
  return built ? built->detach() : obj{};
}

}  // namespace lv
