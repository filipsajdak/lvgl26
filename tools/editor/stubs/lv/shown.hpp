#pragma once

// Editor stub of lv/shown.hpp: a node that follows a state of an enum or a bool and shows the
// tree of its value.
//
//   lv::shown(current)[lv::is<page::home>(home_ui, {.on_shown = &start}), lv::otherwise()]
//   lv::when(online)[online_ui]
//
// The editor does not check that every value has an arm; GCC does.

#include <lv/detail/missing.hpp>
#include <lv/animate.hpp>
#include <lv/computed.hpp>
#include <lv/easing.hpp>
#include <lv/match.hpp>
#include <lv/one_of.hpp>
#include <lv/state.hpp>
#include <lv/tree.hpp>

#include <concepts>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/shown.hpp"> = true;
}  // namespace lv::detail

namespace lv {

namespace detail {
struct shown_slot {};
}  // namespace detail

/// A page whose tree is in a file of its own: constinit const lv::page home_page = lv::page_of<ui>;
struct page {
  std::size_t size;
  std::size_t align;
  const void* tree;
  bool (*mount)(void* memory, lv_obj_t* parent);
  void (*end)(void* memory);
  void (*abandon)(void* memory);
  lv_obj_t* (*root)(const void* memory);
  void (*on_shown)(detail::shown_slot& slot);
  void (*on_front)(detail::shown_slot& slot);
};


namespace detail {

/// How a page enters or leaves: one property of its root, from one value to another.
struct transition_spec {
  int from = 0;
  int to = 0;
};

/// What lv::shown takes beside its state: lv::shown(current, {.enter = slide_in, .leave = slide_out}).
struct shown_options {
  std::size_t alive = 2;
  transition_spec enter{};
  transition_spec leave{};
};

/// What .on_shown and .on_front take: a function void f(lv::held<T>) of the arm's tree T.
struct shown_hook {
  constexpr shown_hook() = default;
  template <const auto& T>
  consteval shown_hook(void (*)(held<T>)) {}
};

/// What an arm takes beside its tree: lv::is<page::home>(home_ui, {.on_shown = &start}).
struct arm_options {
  /// Called after the tree is built, with a handle to it.
  shown_hook on_shown;
  /// Called each time the page comes to the front: after .on_shown when it was just built.
  shown_hook on_front;
};

}  // namespace detail

/// The page of the tree `Tree`, for lv::page. Its hooks are named as those of an arm:
/// lv::page_of<ui, {.on_shown = &start, .on_front = &look}>.
template <const auto& Tree, detail::arm_options Hooks = {}>
  requires tree_node<decltype(Tree)>
inline constexpr page page_of{sizeof(mounted<Tree>), alignof(mounted<Tree>), &Tree, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

namespace detail {

template <class Pattern, class T>
  requires included<"lv/shown.hpp">
struct tree_arm<Pattern, T> {
  const T* tree;
  arm_options options;
};

template <class S>
concept shown_source = dependency<S> && (std::is_enum_v<value_of<S>> || std::same_as<value_of<S>, bool>);

template <class S, class... Arms>
struct shown_node {
  const S* source;

  /// The arms: lv::shown(state)[lv::is<a>(tree), lv::is<b>(), lv::otherwise(other)].
  template <class... A>
    requires(sizeof...(Arms) == 0)
  constexpr shown_node<S, A...> operator[](const A&...) const {
    return {source};
  }
};

template <class S, class... Arms>
inline constexpr bool is_structural<shown_node<S, Arms...>> = true;

template <class S>
struct when_node {
  const S* source;

  /// The tree shown while the flag is true.
  template <class T>
    requires tree_node<T> || std::same_as<T, page>
  constexpr shown_node<S, tree_arm<one_of<true>, T>, tree_arm<one_of<false>, void>> operator[](const T&) const {
    return {source};
  }
};

}  // namespace detail

/// How a page of lv::shown enters or leaves: the arguments of lv::animate, for the page's root.
template <class D = void>
constexpr detail::transition_spec transition(const animatable<"obj">& what, duration time, const easing& how = {}) {
  return {};
}

/// Shows the arm of the value of `state`: lv::shown(current)[lv::is<page::home>(home_ui), ...].
template <detail::shown_source S>
constexpr detail::shown_node<S> shown(const S& state, const detail::shown_options& options = {}) {
  return {&state};
}

/// Shows `tree` while `flag` is true: lv::when(online)[online_ui].
template <detail::shown_source S>
  requires std::same_as<detail::value_of<S>, bool>
constexpr detail::when_node<S> when(const S& flag, const detail::shown_options& options = {}) {
  return {&flag};
}

/// The arms as further arguments: lv::shown(current, lv::is<page::home>(home_ui), ...).
template <detail::shown_source S, class... A>
  requires(sizeof...(A) > 0)
constexpr detail::shown_node<S, A...> shown(const S& state, const A&...) {
  return {&state};
}

/// Options, then the arms: lv::shown(current, {.enter = slide_in}, lv::is<page::home>(home_ui), ...).
template <detail::shown_source S, class... A>
  requires(sizeof...(A) > 0)
constexpr detail::shown_node<S, A...> shown(const S& state, const detail::shown_options& options, const A&...) {
  return {&state};
}

/// The tree as the second argument: lv::when(online, online_ui).
template <detail::shown_source S, class T>
  requires std::same_as<detail::value_of<S>, bool> && tree_node<T>
constexpr detail::shown_node<S, detail::tree_arm<detail::one_of<true>, T>, detail::tree_arm<detail::one_of<false>, void>>
when(const S& flag, const T&) {
  return {&flag};
}

}  // namespace lv
