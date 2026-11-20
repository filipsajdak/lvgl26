#pragma once

// Editor stub of lv/one_of.hpp: one slot for one of several trees.
//
//   constinit lv::one_of<home, settings> window;
//   window.mount<settings>(display.screen());
//   window.visit([](lv::held<home> page) {}, [](auto) {});
//
// The editor does not check that a tree is one of the slot's, nor that every tree has an
// arm; GCC does.

#include <lv/detail/missing.hpp>
#include <lv/tree.hpp>

#include <expected>
#include <optional>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/one_of.hpp"> = true;
}  // namespace lv::detail

namespace lv {

/// The arm of visit for a slot that holds no tree: [](lv::not_mounted) { ... }.
struct not_mounted {};

/// A handle to the tree Tree in an lv::one_of: what an arm of visit gets, and what
/// slot.get<Tree>() gives. It never owns; each use checks that the slot holds Tree.
template <const auto& Tree>
  requires tree_node<decltype(Tree)>
class held {
 public:
  /// A handle to no slot: false.
  constexpr held() = default;
  /// Whether the slot holds Tree now.
  bool has_value() const noexcept { __builtin_unreachable(); }
  explicit operator bool() const noexcept { __builtin_unreachable(); }
  /// The tree, which the slot must hold now.
  mounted<Tree>* operator->() const { __builtin_unreachable(); }
  mounted<Tree>& operator*() const { __builtin_unreachable(); }
};

namespace stub {

// What the arm A returns, called with the first of Args it takes.
template <class A, class... Args>
struct first_result {
  using type = void;
};
template <class A, class X, class... Rest>
struct first_result<A, X, Rest...> {
  using type = typename std::conditional_t<std::is_invocable_v<A&, X>, std::invoke_result<A&, X>,
                                           first_result<A, Rest...>>::type;
};

template <class R, bool Empty>
using visit_out = std::conditional_t<Empty || std::is_void_v<R>, R, std::optional<R>>;

// A type that only a generic arm takes: an arm for the empty slot takes lv::not_mounted
// and not this.
struct any_probe {};

template <class A>
inline constexpr bool empty_arm = std::is_invocable_v<A&, not_mounted> && !std::is_invocable_v<A&, any_probe>;

// No result named: the common type of the arms' results.
struct no_named_result {};

template <class Named, class Common>
using visit_type = std::conditional_t<std::is_same_v<Named, no_named_result>, Common, Named>;

}  // namespace stub

/// One slot for one of the trees, alive one at a time, in the memory of the largest.
template <const auto&... Trees>
  requires detail::tree_nodes<decltype(Trees)...> && detail::included<"lv/one_of.hpp">
class one_of<Trees...> {
 public:
  /// Holds no tree: a constinit global, or a member.
  constexpr one_of() = default;
  one_of(const one_of&) = delete;
  one_of& operator=(const one_of&) = delete;

  /// Ends the tree that is there, then builds the widgets of T under `parent`.
  template <const auto& T>
    requires tree_node<decltype(T)>
  std::expected<void, mount_error> mount(obj parent) { __builtin_unreachable(); }
  /// Deletes the widgets of the tree that is mounted: the slot is empty.
  void unmount() {}

  /// Whether a tree is mounted.
  bool has_value() const noexcept { __builtin_unreachable(); }
  explicit operator bool() const noexcept { __builtin_unreachable(); }
  /// Why no tree is mounted: not_mounted, or the failure of the last mount.
  mount_error error() const { __builtin_unreachable(); }

  /// Whether the tree T is the one mounted.
  template <const auto& T>
    requires tree_node<decltype(T)>
  bool holds() const noexcept { __builtin_unreachable(); }
  /// A handle to the tree T: true while the slot holds T.
  template <const auto& T>
    requires tree_node<decltype(T)>
  held<T> get() noexcept { __builtin_unreachable(); }

  /// Calls the arm whose parameter names the tree that is mounted, with a handle:
  /// lv::held<home>, auto for every other tree, lv::not_mounted for the empty slot. The
  /// result is R of visit<R>, or the common type of the arms' results; in a std::optional
  /// without the arm for the empty slot.
  template <class Named = stub::no_named_result, class... Arms>
  auto visit(Arms&&... arms)
      -> stub::visit_out<stub::visit_type<Named, std::common_type_t<typename stub::first_result<
                                                     std::remove_cvref_t<Arms>, held<Trees>..., not_mounted>::type...>>,
                         (stub::empty_arm<std::remove_cvref_t<Arms>> || ...)> {
    __builtin_unreachable();
  }
};

}  // namespace lv
