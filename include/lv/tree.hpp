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
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <new>
#include <span>
#include <tuple>
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

}  // namespace detail

// A node of a tree: what lv::button({...})[...] and a part that returns one make. Where a
// part takes a piece of a tree to put inside its own.
template <class T>
concept tree_node = detail::is_node<std::remove_cvref_t<T>>;

namespace detail {

// A node that builds no widget of its own but decides which widgets its parent gets, such
// as lv::each (lv/list.hpp). Such a type has a static `build<Tree, Path...>(parent, state)`.
template <class T>
inline constexpr bool is_structural = false;

// What a node takes as a child: a node, or a structural node.
template <class T>
concept tree_child = tree_node<T> || is_structural<std::remove_cvref_t<T>>;

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

// The widgets that lv::for_each made, not yet the children of a parent: the parent's
// operator[] takes each of them in turn into its own children (see splice).
template <class... Nodes>
struct repeated {
  kids<Nodes...> items;
};

template <class T>
inline constexpr bool is_repeated = false;
template <class... Nodes>
inline constexpr bool is_repeated<repeated<Nodes...>> = true;

// What a parent takes in []: a child, or the widgets of a repetition.
template <class T>
concept tree_kid = tree_child<T> || is_repeated<std::remove_cvref_t<T>>;

// References to the children that a value gives its parent: itself, or each widget of a
// repetition.
template <class K>
constexpr std::tuple<const K&> kid_refs(const K& k) {
  return {k};
}
template <class... Nodes>
constexpr std::tuple<const Nodes&...> kid_refs(const repeated<Nodes...>& r) {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::tuple<const Nodes&...>{kid<I>(r.items)...};
  }(std::index_sequence_for<Nodes...>{});
}

// The node `self` (which has no children) with the children `k`, each repetition among
// them replaced by its widgets.
template <class Node, class... K>
constexpr auto splice(const Node& self, const K&... k) {
  return std::apply(
      [&](const auto&... flat) {
        static_assert((std::size_t{0} + ... + std::size_t{is_structural<std::remove_cvref_t<decltype(flat)>>}) <= 1,
                      "lv: a widget holds at most one lv::each. Put each one in a widget of its own: "
                      "lv::column[lv::each(a, &row::id)[...]], lv::column[lv::each(b, &row::id)[...]].");
        return node<Node::kind, std::remove_cvref_t<decltype(flat)>...>{
            self.p, kids<std::remove_cvref_t<decltype(flat)>...>{flat...}, self.bare};
      },
      std::tuple_cat(kid_refs(k)...));
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
  template <detail::tree_child... K>
    requires(sizeof...(Kids) == 0)
  constexpr node<W, K...> operator[](const K&... k) const {
    // Its rows are the widget's children from a fixed place on.
    static_assert((std::size_t{0} + ... + std::size_t{detail::is_structural<K>}) <= 1,
                  "lv: a widget holds at most one lv::each. Put each one in a widget of its own: "
                  "lv::column[lv::each(a, &row::id)[...]], lv::column[lv::each(b, &row::id)[...]].");
    return {p, {k...}, bare};
  }

  // The same with lv::for_each among the children: its widgets become children of this one,
  // in their place.
  template <detail::tree_kid... K>
    requires(sizeof...(Kids) == 0 && (detail::is_repeated<K> || ...))
  constexpr auto operator[](const K&... k) const {
    return detail::splice(*this, k...);
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

  template <tree_kid... Kids>
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
  template <tree_kid... Kids>
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

  template <tree_kid... Kids>
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

// The ids under a structural node belong to what it builds, such as one row of lv::each:
// they are not ids of the tree.
template <class S>
  requires is_structural<S>
consteval void collect(std::vector<id_entry>&, const S&) {}

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
  void* storage = nullptr;  // the run-time storage of the node being built, if it keeps any
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

// ---- run-time storage -------------------------------------------------------
//
// A node kind that keeps data while its widgets exist says so in its own type, with a
// nested template that the tree's position fills in:
//
//   struct my_kind {
//     template <const auto& Tree, std::size_t... Path>
//     struct storage { lv::detail::watch_node watcher; int shown = -1; };
//     template <const auto& Tree, std::size_t... Path>
//     static lv_obj_t* build(lv_obj_t* parent, build_state& st);  // storage_of<Tree, Path...>(st)
//   };
//
// The storage of a tree is one plain object made from these, sized by the compiler: a
// member s<k> for child number k of each widget that has storage below it. lv::mounted
// holds it, so lvgl26 allocates nothing for it. It is value-initialised before the
// widgets are built, and destroyed while they all still exist: a storage destructor lets
// go of what the node holds, and never deletes a widget.

// Whether the kind K declares a member named `storage`.
template <class K>
consteval bool declares_storage() {
  for (auto m : std::meta::members_of(^^K, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == "storage") return true;
  return false;
}

template <class N>
consteval bool keeps_storage() {
  if constexpr (is_structural<N>)
    return declares_storage<N>();
  else
    return false;
}

// Whether the node type N, or a node below it, keeps run-time storage. Asked once per type,
// so that a tree of plain widgets costs one look at each type of node.
template <class N>
inline constexpr bool any_storage = keeps_storage<N>();
template <fixed W, class... Kids>
inline constexpr bool any_storage<node<W, Kids...>> = (false || ... || any_storage<Kids>);

// What the storage of a kind must be: made from nothing, and ended without an error.
template <class K, const auto& Tree, std::size_t... Path>
concept storage_kind = requires { typename K::template storage<Tree, Path...>; } &&
                       std::default_initializable<typename K::template storage<Tree, Path...>> &&
                       std::is_nothrow_destructible_v<typename K::template storage<Tree, Path...>>;

template <const auto& Tree, std::size_t... Path>
struct storage_of_node;

// The run-time storage of the node of `Tree` at Path.
template <const auto& Tree, std::size_t... Path>
using storage_t = typename storage_of_node<Tree, Path...>::type;

// The members of a widget's storage: s<k> for each child k that keeps storage.
template <const auto& Tree, std::size_t... Path>
consteval std::vector<std::meta::info> storage_members() {
  using node_type = std::remove_cvref_t<decltype(descend<Path...>(Tree))>;
  std::vector<std::meta::info> specs;
  template for (constexpr auto k : indices<kid_count<node_type>>) {
    if constexpr (any_storage<std::remove_cvref_t<decltype(descend<Path..., k>(Tree))>>)
      specs.push_back(std::meta::data_member_spec(^^storage_t<Tree, Path..., k>, {.name = "s" + decimal(k)}));
  }
  return specs;
}

template <const auto& Tree, std::size_t... Path>
struct widget_storage {
  struct type;
  consteval { std::meta::define_aggregate(^^type, storage_members<Tree, Path...>()); }
};

template <class K, const auto& Tree, std::size_t... Path>
consteval std::meta::info kind_storage() {
  if constexpr (storage_kind<K, Tree, Path...>) {
    return ^^typename K::template storage<Tree, Path...>;
  } else {
    static_assert(false,
                  "lv: a node kind with a member `storage` declares it as "
                  "`template <const auto& Tree, std::size_t... Path> struct storage { ... };`, "
                  "default-constructible and with a destructor that does not throw.");
    return ^^void;
  }
}

template <const auto& Tree, std::size_t... Path>
struct storage_of_node {
  using node_type = std::remove_cvref_t<decltype(descend<Path...>(Tree))>;
  static consteval std::meta::info find() {
    if constexpr (is_structural<node_type>)
      return kind_storage<node_type, Tree, Path...>();
    else
      return ^^typename widget_storage<Tree, Path...>::type;
  }
  using type = [:find():];
};

// The storage of the empty tree: a tree whose nodes keep nothing.
struct no_storage {};

template <const auto& Tree>
using tree_storage = std::conditional_t<any_storage<std::remove_cvref_t<decltype(Tree)>>,
                                        storage_t<Tree>, no_storage>;

// Where the storage of child K of the widget at Path lies in the widget's storage.
template <const auto& Tree, std::size_t K, std::size_t... Path>
consteval std::size_t locate() {
  for (auto m : std::meta::nonstatic_data_members_of(^^storage_t<Tree, Path...>,
                                                    std::meta::access_context::unchecked()))
    if (std::meta::identifier_of(m) == "s" + decimal(K)) return static_cast<std::size_t>(std::meta::offset_of(m).bytes);
  fail("lv: internal: no storage for child " + decimal(K));
}

// The storage of the node being built: what a kind's `build` uses.
template <const auto& Tree, std::size_t... Path>
storage_t<Tree, Path...>& storage_of(const build_state& st) {
  return *static_cast<storage_t<Tree, Path...>*>(st.storage);
}

// Whether a structural node lies on the way from `n` down the path Path, such as the
// lv::each whose row template holds the node at the end of it.
template <class Node>
consteval bool structural_above(const Node&) {
  return false;
}
template <std::size_t First, std::size_t... Rest, class Node>
consteval bool structural_above(const Node& n) {
  if constexpr (is_structural<Node>)
    return true;
  else
    return structural_above<Rest...>(kid<First>(n.kids));
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

// A state of an enum follows member Member of the node of `Tree` at Path, the choice of its
// options. The options are the node's own, and must come from the same enum.
template <const auto& Tree, std::size_t Member, std::size_t... Path>
void follow_enum_choice(lv_obj_t* w) {
  constexpr const auto& N = descend<Path...>(Tree);
  constexpr auto W = std::remove_cvref_t<decltype(N)>::kind;
  if constexpr (option_member<W> >= members_of_kept<W>.size() || binder<targets<W>[Member]> == std::meta::info{}) {
    static_assert(false,
                  "lv: a state of an enum follows only the choice of a dropdown or a roller: "
                  ".selected = lv::bind(state), beside .options = lv::options_of<the same enum>.");
  } else {
    constexpr const auto& given = N.p.[:members_of_kept<W>[option_member<W>]:];
    constexpr const auto& chosen = N.p.[:members_of_kept<W>[Member]:];
    constexpr bool checked = (check_enum_choice(given.set, given.v.v0, chosen.bound_format), true);
    static_assert(checked);
    // Its own storage: the address of the tree's would keep the whole tree in the program.
    static constexpr option_list options = given.v.v0;
    static constexpr enum_link link{chosen.bound, chosen.bound_ops, &options};
    bind_choice<W, Member>(w, link);
  }
}

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
      add_const_style<style_at<Tree, i, Path...>, selector_of<targets<W>[i]>()>(w);
    } else {
      constexpr auto a = N.p.[:m:];  // a constant: the tree itself is not kept
      if constexpr (a.follows == follows_row_field) {
        // lv::each sets it with the row's value, after the row is built.
        static_assert(structural_above<Path...>(Tree),
                      "lv: lv::bind(&T::field) follows a field of a row, and this node is not "
                      "in a row. Use it in the row template of lv::each: "
                      "lv::each(tracks, &track::id)[lv::label({.text = lv::bind(&track::title)})].");
      } else if constexpr (a.follows == follows_choice) {
        follow_enum_choice<Tree, i, Path...>(w);
      } else {
        static_assert(a.follows == follows_nothing || bindable<W, i>(),
                      "lv: this property cannot follow a state. LVGL has no bind function "
                      "for it and it takes more than one value.");
        apply_member<W, i, std::remove_cvref_t<decltype(a)>, true>(w, a);
      }
    }
  }
}

template <const auto& Tree, std::size_t... Path>
lv_obj_t* build_widget(lv_obj_t* parent, build_state& st);

// Builds one node of the constexpr tree `Tree` under `parent`, with its children. A
// structural node builds what it decides on.
template <const auto& Tree, std::size_t... Path>
lv_obj_t* build(lv_obj_t* parent, build_state& st) {
  using node_type = std::remove_cvref_t<decltype(descend<Path...>(Tree))>;
  if constexpr (is_structural<node_type>)
    return node_type::template build<Tree, Path...>(parent, st);
  else
    return build_widget<Tree, Path...>(parent, st);
}

template <const auto& Tree, std::size_t... Path>
lv_obj_t* build_widget(lv_obj_t* parent, build_state& st) {
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
    if constexpr (any_storage<std::remove_cvref_t<decltype(descend<Path..., i>(Tree))>>) {
      void* own = st.storage;
      st.storage = static_cast<std::byte*>(own) + locate<Tree, i, Path...>();
      if (st.ok) build<Tree, Path..., i>(w.raw(), st);
      st.storage = own;
    } else {
      if (st.ok) build<Tree, Path..., i>(w.raw(), st);
    }
  }
  return w.raw();
}

// The storage of a tree inside the object that holds it: alive from the start of a mount
// until the root is deleted. A tree that keeps nothing holds nothing.
template <class S>
class storage_place {
  struct none {};
  union {
    none none_{};  // active until start(), so that a constinit holder is constant
    S value_;
  };

 public:
  constexpr storage_place() {}
  storage_place(const storage_place&) = delete;
  storage_place& operator=(const storage_place&) = delete;
  constexpr ~storage_place() {}

  void* start() { return std::construct_at(&value_); }
  void end() noexcept { std::destroy_at(&value_); }
};

template <>
class storage_place<no_storage> {
 public:
  constexpr storage_place() = default;
  storage_place(const storage_place&) = delete;
  storage_place& operator=(const storage_place&) = delete;

  void* start() { return nullptr; }
  void end() noexcept {}
};

}  // namespace detail

// ---- the mounted tree -------------------------------------------------------

enum class mount_error {
  out_of_memory,  // LVGL could not create a widget
  not_mounted,    // an lv::mounted made empty, whose mount(parent) has not run
};

// The widgets built from a tree, and the run-time storage of its nodes. Owns the
// widgets: they are deleted with this object.
//
// It is neither copied nor moved: LVGL's handler on the root, and what the nodes keep,
// point into it.
// It is built where it lives, and carries its own failure:
//   auto root = lv::mount<ui>(display.screen());   // on the stack, built in place
//   if (!root) return root.error();
//   root->get<"title">().set({.text = "Ready"});
//
//   constinit lv::mounted<ui> page;               // a global, mounted later
//   if (!page.mount(display.screen())) ...
//
// sizeof and alignof are constants: lv::mount_in builds one in memory of the caller's.
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
class mounted {
  lv_obj_t* root_ = nullptr;
  mount_error error_ = mount_error::not_mounted;
  bool built_ = false;
  std::array<lv_obj_t*, detail::id_count<Tree>> ids_{};
  [[no_unique_address]] detail::storage_place<detail::tree_storage<Tree>> storage_;

  static constexpr bool keeps_storage = detail::any_storage<std::remove_cvref_t<decltype(Tree)>>;

  // LVGL deletes the root, from here or from outside: the storage ends while every
  // widget of the tree still exists.
  static void root_deleted(lv_event_t* e) {
    auto* self = static_cast<mounted*>(lv_event_get_user_data(e));
    self->storage_.end();
    self->root_ = nullptr;
  }

  std::expected<void, mount_error> build(obj parent) {
    detail::build_state st{ids_.data(), true, storage_.start()};
    lv_obj_t* root = detail::build<Tree>(parent.raw(), st);
    if (st.ok && root != nullptr &&
        lv_obj_add_event_cb(root, &root_deleted, LV_EVENT_DELETE, this) != nullptr) {
      root_ = root;
      built_ = true;
      return {};
    }
    storage_.end();                         // first the storage,
    if (root != nullptr) lv_obj_delete(root);  // then a half-built tree
    error_ = mount_error::out_of_memory;
    return std::unexpected(error_);
  }

 public:
  // Holds no widgets until mount(parent): a constinit global, or a member.
  constexpr mounted() = default;

  // Builds the widgets of `Tree` under `parent`, here. Check the result with operator bool.
  explicit mounted(obj parent)
    pre(parent.raw() != nullptr)
  {
    (void)build(parent);
  }

  mounted(const mounted&) = delete;
  mounted& operator=(const mounted&) = delete;

  ~mounted() {
    if (root_ != nullptr) lv_obj_delete(root_);  // root_deleted ends the storage
  }

  // Builds the widgets of `Tree` under `parent`, for an lv::mounted made empty.
  std::expected<void, mount_error> mount(obj parent)
    pre(parent.raw() != nullptr)
    pre(!built_)
  {
    return build(parent);
  }

  // Whether the widgets were built. They stay built when the parent deletes them later:
  // root() is then empty.
  explicit operator bool() const noexcept { return built_; }

  // Why the widgets were not built.
  mount_error error() const
    pre(!built_)
  {
    return error_;
  }

  // Shaped like std::expected, so that root->get<"id">() reads as before.
  mounted* operator->()
    pre(built_)
  {
    return this;
  }
  const mounted* operator->() const
    pre(built_)
  {
    return this;
  }
  mounted& operator*()
    pre(built_)
  {
    return *this;
  }
  const mounted& operator*() const
    pre(built_)
  {
    return *this;
  }

  // The root widget, or an empty view if its parent already deleted it.
  obj root() const { return obj{root_}; }

  // Gives the widgets to their parent: they now live until the parent is deleted,
  // as widgets made with LVGL's C API do. Returns the root.
  obj detach()
    requires(!keeps_storage)
  {
    lv_obj_t* root = std::exchange(root_, nullptr);
    if (root != nullptr) lv_obj_remove_event_cb_with_user_data(root, &root_deleted, this);
    return obj{root};
  }
  obj detach() = delete(
      "lv: this tree keeps run-time storage inside its lv::mounted, so its widgets cannot "
      "outlive it. Keep the lv::mounted as long as the widgets are shown.");

  // A typed view of the node with this id.
  template <detail::fixed Name>
  auto get() const {
    // A check in the body, not pre(): GCC 16.2 crashes on pre() with a deduced return type.
    contract_assert(root_ != nullptr && "the mounted tree was deleted by its parent");
    using view_type = typename[:detail::id_view<Tree>(Name.sv()):];
    return view_type{ids_[detail::id_index<Tree>(Name.sv())]};
  }
};

// The bytes that lv::mount_in needs for `Tree`: sizeof(lv::mounted<Tree>), a constant.
// Its alignment is alignof(lv::mounted<Tree>).
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
inline constexpr std::size_t storage_size = sizeof(mounted<Tree>);

// Builds the widgets of the constexpr tree `Tree` under `parent`. The result is built in
// the caller's variable, and carries its failure: `if (!root) ... root.error()`.
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
mounted<Tree> mount(obj parent)
  pre(parent.raw() != nullptr)
{
  return mounted<Tree>{parent};
}

// The same, built in `memory` of the caller's: a region laid out at start, a buffer of
// a screen. The caller ends it with std::destroy_at, which deletes the widgets.
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
mounted<Tree>& mount_in(std::span<std::byte> memory, obj parent)
  pre(parent.raw() != nullptr)
  pre(memory.size() >= sizeof(mounted<Tree>))
  pre(reinterpret_cast<std::uintptr_t>(memory.data()) % alignof(mounted<Tree>) == 0)
{
  return *::new (static_cast<void*>(memory.data())) mounted<Tree>(parent);
}

// Builds the widgets of `Tree` under `parent` and leaves them to it, as widgets
// made with LVGL's C API are: they live until the parent is deleted.
// Returns the root, or an empty view if LVGL had no memory.
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
obj place(obj parent)
  pre(parent.raw() != nullptr)
{
  static_assert(!detail::any_storage<std::remove_cvref_t<decltype(Tree)>>,
                "lv: lv::place leaves the widgets to their parent, and this tree keeps run-time "
                "storage that must live as long as they do. Keep it in an lv::mounted: "
                "lv::mount<ui>(parent), or lv::mount_in<ui>(memory, parent) for memory of your own.");
  mounted<Tree> built{parent};
  return built ? built.detach() : obj{};
}

}  // namespace lv
