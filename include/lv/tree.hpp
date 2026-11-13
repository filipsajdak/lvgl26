#pragma once

// L2: a UI described as a constexpr value, and lv::mount() to build it.
//
//   constexpr auto ui = lv::node<"obj">(
//       lv::node<"label">(lv::id<"title">, lv::p<"text">("Speed")),
//       lv::node<"arc">(lv::id<"speedo">, lv::p<"range">(0, 240)));
//
//   auto root = lv::mount(ui, lv::screen_active());
//   root->get<"speedo">().set<"value">(120);

#include <lv/owned.hpp>
#include <lv/view.hpp>

#include <array>
#include <expected>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv {

// ---- the pieces of a tree ---------------------------------------------------

template <detail::fixed Name>
struct id_t {};

// Names a node, so that the mounted tree can hand out a typed view of it.
template <detail::fixed Name>
inline constexpr id_t<Name> id{};

template <detail::fixed P, class... A>
struct prop_t {
  std::tuple<A...> args;
};

// A property and its arguments: p<"range">(0, 240) becomes set<"range">(0, 240).
template <detail::fixed P, class... A>
constexpr prop_t<P, std::decay_t<A>...> p(A&&... a) {
  return {{std::forward<A>(a)...}};
}

template <lv_event_code_t Code, class F>
struct handler_t {
  F f;
};

// An event handler: on<LV_EVENT_CLICKED>(&reset).
template <lv_event_code_t Code, detail::event_handler F>
constexpr handler_t<Code, F> on(F f) {
  return {std::move(f)};
}

template <detail::fixed W, class... Items>
struct node_t {
  std::tuple<Items...> items;
};

// A widget with its ids, properties, handlers and child nodes, in any order.
template <detail::fixed W, class... Items>
constexpr node_t<W, std::decay_t<Items>...> node(Items&&... items) {
  return {{std::forward<Items>(items)...}};
}

// ---- compile-time facts about a tree ----------------------------------------

namespace detail {

struct id_entry {
  std::string_view name;
  std::meta::info view;  // the widget<...> type of the node
};

template <fixed W, fixed Name>
consteval void collect_item(std::vector<id_entry>& out, std::type_identity<id_t<Name>>,
                            std::type_identity<widget<W>>) {
  out.push_back({Name.sv(), ^^widget<W>});
}

template <fixed W, class Item>
consteval void collect_item(std::vector<id_entry>&, std::type_identity<Item>,
                            std::type_identity<widget<W>>) {}

template <fixed W, class... Items>
consteval void collect(std::vector<id_entry>& out, std::type_identity<node_t<W, Items...>>);

template <fixed Parent, fixed W, class... Items>
consteval void collect_item(std::vector<id_entry>& out,
                            std::type_identity<node_t<W, Items...>> child,
                            std::type_identity<widget<Parent>>) {
  collect(out, child);
}

template <fixed W, class... Items>
consteval void collect(std::vector<id_entry>& out, std::type_identity<node_t<W, Items...>>) {
  (collect_item(out, std::type_identity<Items>{}, std::type_identity<widget<W>>{}), ...);
}

// The ids of a tree, in the order mount() meets them.
template <class Tree>
consteval std::vector<id_entry> ids_of() {
  std::vector<id_entry> out;
  collect(out, std::type_identity<Tree>{});
  for (std::size_t i = 0; i < out.size(); ++i)
    for (std::size_t j = i + 1; j < out.size(); ++j)
      if (out[i].name == out[j].name)
        throw unknown_name("lv: the id '" + std::string(out[i].name) +
                           "' is used twice in one tree. Ids must be unique.");
  return out;
}

template <class Tree>
inline constexpr std::size_t id_count = ids_of<Tree>().size();

template <class Tree>
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

template <class Tree>
consteval std::meta::info id_view(std::string_view name) {
  return ids_of<Tree>()[id_index<Tree>(name)].view;
}

// ---- building ---------------------------------------------------------------

struct build_state {
  lv_obj_t** next_id;
  bool ok = true;
};

template <fixed W, class... Items>
lv_obj_t* build(const node_t<W, Items...>& n, lv_obj_t* parent, build_state& st);

template <fixed W, fixed Name>
void apply(widget<W> w, const id_t<Name>&, build_state& st) {
  *st.next_id++ = w.raw();
}

template <fixed W, fixed P, class... A>
void apply(widget<W> w, const prop_t<P, A...>& pr, build_state&) {
  std::apply([w](const A&... a) { w.template set<P>(a...); }, pr.args);
}

template <fixed W, lv_event_code_t Code, class F>
void apply(widget<W> w, const handler_t<Code, F>& h, build_state&) {
  w.template on<Code>(h.f);
}

template <fixed W, fixed C, class... Items>
void apply(widget<W> w, const node_t<C, Items...>& child, build_state& st) {
  build(child, w.raw(), st);
}

template <fixed W, class... Items>
lv_obj_t* build(const node_t<W, Items...>& n, lv_obj_t* parent, build_state& st) {
  if (!st.ok) return nullptr;
  auto w = widget<W>::create(obj{parent});
  if (!w) {
    st.ok = false;
    return nullptr;
  }
  std::apply([&](const Items&... item) { ((st.ok ? apply(w, item, st) : void()), ...); },
             n.items);
  return w.raw();
}

}  // namespace detail

// ---- the mounted tree -------------------------------------------------------

enum class mount_error {
  out_of_memory,  // LVGL could not create a widget
};

// The widgets built from a tree. Owns them: they are deleted with this object.
template <class Tree>
class mounted {
  owned<obj> root_;
  std::array<lv_obj_t*, detail::id_count<Tree>> ids_{};

  template <class T>
  friend std::expected<mounted<T>, mount_error> mount(const T&, obj);

 public:
  mounted() = default;
  mounted(mounted&&) noexcept = default;
  mounted& operator=(mounted&&) noexcept = default;

  // The root widget, or an empty view if its parent already deleted it.
  obj root() const { return root_.get(); }

  // A typed view of the node named by lv::id<"name">.
  template <detail::fixed Name>
  auto get() const {
    contract_assert(root_ && "the mounted tree was deleted by its parent");
    using view = typename[:detail::id_view<Tree>(Name.sv()):];
    return view{ids_[detail::id_index<Tree>(Name.sv())]};
  }
};

// Builds the widgets of `tree` under `parent`.
template <class Tree>
std::expected<mounted<Tree>, mount_error> mount(const Tree& tree, obj parent) {
  contract_assert(parent.raw() != nullptr);
  mounted<Tree> m;
  detail::build_state st{m.ids_.data()};
  lv_obj_t* root = detail::build(tree, parent.raw(), st);
  m.root_ = owned<obj>{obj{root}};  // also deletes a half-built tree on failure
  if (!st.ok) return std::unexpected(mount_error::out_of_memory);
  return m;
}

}  // namespace lv
