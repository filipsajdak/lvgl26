#pragma once

// Editor stub of lv/one_of.hpp: one slot for one of several trees.
//
//   constinit lv::one_of<home, settings> window;
//   window.mount<settings>(display.screen());
//   window.visit([](lv::mounted<home>&) {}, [](auto&) {});
//
// The editor does not check that a tree is one of the slot's, nor that every tree has an
// arm; GCC does.

#include <lv/tree.hpp>

#include <expected>
#include <optional>
#include <type_traits>

namespace lv {

/// The arm of visit for a slot that holds no tree: [](lv::not_mounted) { ... }.
struct not_mounted {};

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

}  // namespace stub

/// One slot for one of the trees, alive one at a time, in the memory of the largest.
template <const auto&... Trees>
  requires(tree_node<decltype(Trees)> && ...)
class one_of {
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
  /// The tree T, which must be the one mounted.
  template <const auto& T>
    requires tree_node<decltype(T)>
  mounted<T>& get() { __builtin_unreachable(); }

  /// Calls the arm whose parameter names the tree that is mounted: lv::mounted<home>&,
  /// auto& for every other tree, lv::not_mounted for the empty slot.
  template <class... Arms>
  auto visit(Arms&&... arms)
      -> stub::visit_out<typename stub::first_result<std::remove_cvref_t<Arms...[0]>, not_mounted,
                                                     mounted<Trees>&...>::type,
                         (std::is_invocable_v<std::remove_cvref_t<Arms>&, not_mounted> || ...)> {
    __builtin_unreachable();
  }
};

}  // namespace lv
