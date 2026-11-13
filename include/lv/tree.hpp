#pragma once

// L2: a UI described as a constexpr value, and lv::mount to build it.
//
//   constexpr auto ui = lv::column({.size = {320_px, 240_px}})[
//       lv::label({.id = "title", .text = "Speed"}),
//       lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
//       lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];
//
//   auto root = lv::mount<ui>(lv::screen_active());
//   root->get<"speedo">().set({.value = 0});
//
// Properties go in (), in alphabetical order; children go in [].
// The tree is read at compile time. Only the calls for the properties that are
// set end up in the program; the tree itself is not stored in it.

#include <lv/owned.hpp>
#include <lv/props.hpp>
#include <lv/view.hpp>

#include <array>
#include <expected>
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

// The children of a node. A plain recursive struct: every child has its own
// member path, which a reference template argument needs to be told apart.
template <class... Kids>
struct kids_list {};

template <class First, class... Rest>
struct kids_list<First, Rest...> {
  First head;
  kids_list<Rest...> tail;
};

constexpr kids_list<> make_kids() { return {}; }

template <class First, class... Rest>
constexpr kids_list<First, Rest...> make_kids(First first, Rest... rest) {
  return {std::move(first), make_kids(std::move(rest)...)};
}

template <std::size_t I, class First, class... Rest>
constexpr const auto& kid(const kids_list<First, Rest...>& l) {
  if constexpr (I == 0)
    return l.head;
  else
    return kid<I - 1>(l.tail);
}

// props<W>, named so that it is generated on first use: see props_of.
template <fixed W, class D>
using lazy_props = typename props_of<W, D>::type;

}  // namespace detail

// One widget of a tree: its properties and its children.
template <detail::fixed W, class... Kids>
struct node {
  static constexpr auto kind = W;
  props<W> p{};
  detail::kids_list<Kids...> kids{};

  // The children: lv::column()[lv::label(...), lv::arc(...)].
  template <detail::tree_node... K>
    requires(sizeof...(Kids) == 0)
  constexpr node<W, K...> operator[](K... k) const {
    return {p, detail::make_kids(std::move(k)...)};
  }

  constexpr std::string_view id() const {
    return p.id.set ? std::get<0>(p.id.v) : std::string_view{};
  }
};

// Any LVGL widget by name: lv::widget<"spinbox">({...}).
template <detail::fixed W, class D = void>
constexpr node<W> widget(detail::lazy_props<W, D> p = {}) {
  return {std::move(p)};
}

// Short names for the common widgets. C++ cannot generate these names, so this
// list is written by hand; lv::widget<"..."> reaches every other widget.
// Each has exactly one signature, so a wrong value gets a precise error.
#define LV_DETAIL_WIDGET(name, kind)                                   \
  template <class D = void>                                            \
  constexpr node<kind> name(detail::lazy_props<kind, D> p = {}) {      \
    return {std::move(p)};                                             \
  }

LV_DETAIL_WIDGET(box, "obj")
LV_DETAIL_WIDGET(label, "label")
LV_DETAIL_WIDGET(button, "button")
LV_DETAIL_WIDGET(arc, "arc")
LV_DETAIL_WIDGET(bar, "bar")
LV_DETAIL_WIDGET(slider, "slider")
LV_DETAIL_WIDGET(toggle, "switch")
LV_DETAIL_WIDGET(checkbox, "checkbox")
LV_DETAIL_WIDGET(image, "image")
LV_DETAIL_WIDGET(textarea, "textarea")
LV_DETAIL_WIDGET(dropdown, "dropdown")
LV_DETAIL_WIDGET(roller, "roller")
LV_DETAIL_WIDGET(spinner, "spinner")

#undef LV_DETAIL_WIDGET

// A box laid out as a flex column, unless .flex_flow says otherwise.
template <class D = void>
constexpr node<"obj"> column(detail::lazy_props<"obj", D> p = {}) {
  if (!p.flex_flow.set) p.flex_flow = LV_FLEX_FLOW_COLUMN;
  return {std::move(p)};
}

// A box laid out as a flex row, unless .flex_flow says otherwise.
template <class D = void>
constexpr node<"obj"> row(detail::lazy_props<"obj", D> p = {}) {
  if (!p.flex_flow.set) p.flex_flow = LV_FLEX_FLOW_ROW;
  return {std::move(p)};
}

// ---- compile-time facts about a tree ----------------------------------------

namespace detail {

struct id_entry {
  std::string_view name;
  std::meta::info view;  // the view<...> type of the node
};

consteval void collect_kids(std::vector<id_entry>&, const kids_list<>&) {}

template <class First, class... Rest>
consteval void collect_kids(std::vector<id_entry>& out, const kids_list<First, Rest...>& l);

template <fixed W, class... Kids>
consteval void collect(std::vector<id_entry>& out, const node<W, Kids...>& n) {
  if (!n.id().empty()) out.push_back({n.id(), ^^view<W>});
  collect_kids(out, n.kids);
}

template <class First, class... Rest>
consteval void collect_kids(std::vector<id_entry>& out, const kids_list<First, Rest...>& l) {
  collect(out, l.head);
  collect_kids(out, l.tail);
}

// The ids of a tree, in the order mount meets them.
template <const auto& Tree>
consteval std::vector<id_entry> ids_of() {
  std::vector<id_entry> out;
  collect(out, Tree);
  for (std::size_t i = 0; i < out.size(); ++i)
    for (std::size_t j = i + 1; j < out.size(); ++j)
      if (out[i].name == out[j].name)
        throw unknown_name("lv: the id '" + std::string(out[i].name) +
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
  throw unknown_name("lv: the tree has no id '" + std::string(name) + "'. Its ids: " +
                     (valid.empty() ? std::string("none") : valid) + ".");
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

// Builds the node `N`, a part of a constexpr tree, under `parent`.
template <const auto& N>
lv_obj_t* build(lv_obj_t* parent, build_state& st) {
  using node_type = std::remove_cvref_t<decltype(N)>;
  constexpr auto W = node_type::kind;

  auto w = view<W>::create(obj{parent});
  if (!w) {
    st.ok = false;
    return nullptr;
  }
  if constexpr (!N.id().empty()) *st.next_id++ = w.raw();

  template for (constexpr auto i : indices<members_of_props<W>.size()>) {
    constexpr auto m = members_of_props<W>[i];
    if constexpr (N.p.[:m:].set) {
      constexpr auto a = N.p.[:m:];  // a constant: the tree itself is not kept
      apply_member<W, i>(w.raw(), a);
    }
  }

  template for (constexpr auto i : indices<kid_count<node_type>>) {
    if (st.ok) build<kid<i>(N.kids)>(w.raw(), st);
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
class mounted {
  owned<obj> root_;
  std::array<lv_obj_t*, detail::id_count<Tree>> ids_{};

  template <const auto& T>
  friend std::expected<mounted<T>, mount_error> mount(obj);

 public:
  mounted() = default;
  mounted(mounted&&) noexcept = default;
  mounted& operator=(mounted&&) noexcept = default;

  // The root widget, or an empty view if its parent already deleted it.
  obj root() const { return root_.get(); }

  // A typed view of the node with this id.
  template <detail::fixed Name>
  auto get() const {
    contract_assert(root_ && "the mounted tree was deleted by its parent");
    using view_type = typename[:detail::id_view<Tree>(Name.sv()):];
    return view_type{ids_[detail::id_index<Tree>(Name.sv())]};
  }
};

// Builds the widgets of the constexpr tree `Tree` under `parent`.
template <const auto& Tree>
std::expected<mounted<Tree>, mount_error> mount(obj parent) {
  contract_assert(parent.raw() != nullptr);
  mounted<Tree> m;
  detail::build_state st{m.ids_.data()};
  lv_obj_t* root = detail::build<Tree>(parent.raw(), st);
  m.root_ = owned<obj>{obj{root}};  // also deletes a half-built tree on failure
  if (!st.ok) return std::unexpected(mount_error::out_of_memory);
  return m;
}

}  // namespace lv
