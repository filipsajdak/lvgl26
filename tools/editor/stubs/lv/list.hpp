#pragma once

// Editor stub of lv/list.hpp: rows that change, shown by lv::each.
//
//   constinit lv::list<track, 20> tracks;
//   constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::label({.text = lv::bind(&track::title)})]];
//   tracks.push_back({{1}, "Intro"});

#include <lv/detail/missing.hpp>
#include <lv/state.hpp>
#include <lv/tree.hpp>

#include <concepts>
#include <cstddef>
#include <expected>
#include <string>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/list.hpp"> = true;
}  // namespace lv::detail

namespace lv {

/// Why a change to an lv::list did not happen, or is not shown.
enum class list_error {
  full,           // the list holds as many rows as it can; nothing changed
  out_of_memory,  // LVGL had no memory for a row's widgets
};

/// A list of up to N rows of type T: the model of lv::each. A row keeps its address
/// from insert to erase. Make it a `constinit` global, as a state is.
template <class T, std::size_t N>
  requires detail::included<"lv/list.hpp">
class list<T, N> {
  state<counter> count_{};

 public:
  using value_type = T;

  constexpr list() {}
  list(const list&) = delete;
  list& operator=(const list&) = delete;
  ~list() {}

  /// The most rows it holds.
  static constexpr std::size_t capacity() { return N; }
  std::size_t size() const { __builtin_unreachable(); }
  bool empty() const { __builtin_unreachable(); }
  /// The number of rows as a state, to bind to or compute from.
  constexpr const state<counter>& count() const { return count_; }
  const T& operator[](std::size_t) const { __builtin_unreachable(); }
  const T* begin() const { __builtin_unreachable(); }
  const T* end() const { __builtin_unreachable(); }

  /// Adds a row before row `at`. list_error::full or out_of_memory: nothing changed.
  [[nodiscard]] std::expected<void, list_error> insert(std::size_t at, T value) { __builtin_unreachable(); }
  [[nodiscard]] std::expected<void, list_error> push_back(T value) { __builtin_unreachable(); }
  /// Removes row `at`. Every view deletes its widgets.
  void erase(std::size_t at) {}
  /// Moves row `from` to place `to`. Every view moves its widgets.
  void move(std::size_t from, std::size_t to) {}
  /// Changes row `at` with `f(row)`; every view sets that row's bound properties again.
  template <class F>
    requires std::invocable<F&, T&>
  void update(std::size_t at, F f) {}
  /// Replaces all rows. A view keeps the widgets of each key it finds again.
  template <class R>
  [[nodiscard]] std::expected<void, list_error> assign(R&& rows) { __builtin_unreachable(); }
  /// Removes all rows.
  void clear() {}
  /// True while the views follow a change: the list must not change then.
  bool telling() const { __builtin_unreachable(); }
};

namespace detail {

template <class F>
struct field_type {
  using type = F;
};
template <>
struct field_type<std::string> {
  using type = const char*;
};
template <std::size_t M>
struct field_type<char[M]> {
  using type = const char*;
};

/// What lv::each takes after the key: {.on_row = &start, .selected = lv::bind(current)}.
template <class T, class K>
struct each_options {
  void (*on_row)(obj row, const T& value) = nullptr;
  binding<K> selected{};
};

template <class S, class T, class K, class... Row>
struct each_node {
  /// The row template: lv::each(tracks, &track::id)[lv::label(...)].
  template <tree_node R>
    requires(sizeof...(Row) == 0)
  constexpr each_node<S, T, K, R> operator[](const R&) const {
    return {};
  }
};

template <class S, class T, class K, class... Row>
inline constexpr bool is_structural<each_node<S, T, K, Row...>> = true;

}  // namespace detail

/// Binds a property of a row template to a field of the row: .text = lv::bind(&track::title).
template <class C, class F>
  requires(!std::is_function_v<F>)
consteval detail::row_binding<typename detail::field_type<F>::type> bind(F C::*) {
  return {};
}

/// Binds a text property of a row template to a number field: lv::bind(&track::plays, "{} plays").
template <class C, class F>
  requires(!std::is_function_v<F>)
consteval detail::row_formatted_binding<F> bind(F C::*, text_format) {
  return {};
}

/// A row of the row template for each row of `rows`, named by the key `key`:
///   lv::each(tracks, &track::id)[row template]
///   lv::each(tracks, &track::id, {.on_row = &start, .selected = lv::bind(current)})[row template]
template <class S, class T, class K>
  requires std::same_as<typename S::value_type, T>
constexpr detail::each_node<S, T, K> each(const S& rows, K T::* key, detail::each_options<T, K> options = {}) {
  return {};
}

/// The row template as the last argument: lv::each(tracks, &track::id, {...}, track_row).
template <class S, class T, class K, class R>
  requires std::same_as<typename S::value_type, T> && tree_node<R>
constexpr detail::each_node<S, T, K, R> each(const S& rows, K T::* key, detail::each_options<T, K> options, const R& row) {
  return {};
}

/// Without options: lv::each(tracks, &track::id, track_row).
template <class S, class T, class K, class R>
  requires std::same_as<typename S::value_type, T> && tree_node<R>
constexpr detail::each_node<S, T, K, R> each(const S& rows, K T::* key, const R& row) {
  return {};
}

/// The widgets of one row, by the ids of its row template: lv::row_view<card>{row}.get<"title">().
template <const auto& Template>
  requires tree_node<decltype(Template)>
class row_view {
 public:
  constexpr explicit row_view(obj root) {}

  /// The root widget of the row.
  obj root() const { __builtin_unreachable(); }

  /// A typed view of the widget with this id in the row.
  template <detail::fixed Name>
    requires stub::has_id<Template, Name>
  stub::view_of_id<Template, Name> get() const { __builtin_unreachable(); }
};

}  // namespace lv
