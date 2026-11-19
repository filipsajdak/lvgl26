#pragma once

// Editor stub of lv/view.hpp: a view of one widget.

#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/events.hpp>
#include <lv/props.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

namespace lv {

template <detail::fixed W>
  requires detail::known_widget<W>
class view;

/// A view of any widget.
using obj = view<"obj">;

namespace detail {

template <class T>
inline constexpr bool is_view = false;
template <fixed W>
inline constexpr bool is_view<view<W>> = true;

}  // namespace detail

/// A view of some widget: lv::view<"label">, lv::obj. Where a part takes a widget.
template <class T>
concept view_value = detail::is_view<T>;

/// Something that handles an event: a function, or a lambda, that takes an lv::event or
/// nothing. What view.on<>() takes and what a part passes on to `.on_clicked`.
template <class F>
concept event_handler = std::invocable<F&, event> || std::invocable<F&>;

namespace detail {

template <fixed W, fixed P, class... A>
concept has_getter = requires(lv_obj_t* o, A&&... a) { stub::getter<W, P>::call(o, std::forward<A>(a)...); };

// What get<"name">() returns: what the widget's own LVGL getter returns, or the one
// of lv_obj. A name the reference does not list gives stub::anything.
template <fixed W, fixed P, class... A>
struct got {
  using type = stub::anything;
};
template <fixed W, fixed P, class... A>
  requires has_getter<W, P, A...>
struct got<W, P, A...> {
  using type = decltype(stub::getter<W, P>::call(std::declval<lv_obj_t*>(), std::declval<A>()...));
};
template <fixed W, fixed P, class... A>
  requires(!has_getter<W, P, A...>) && has_getter<"obj", P, A...>
struct got<W, P, A...> {
  using type = decltype(stub::getter<"obj", P>::call(std::declval<lv_obj_t*>(), std::declval<A>()...));
};

}  // namespace detail

/// A non-owning, pointer-sized view of an LVGL widget. Copy it freely.
/// It never deletes the widget: see lv::owned and lv::mount for that.
template <detail::fixed W>
  requires detail::known_widget<W>
class view {
  lv_obj_t* p_ = nullptr;

 public:
  /// The name LVGL gives this kind of widget.
  static constexpr std::string_view kind = W.sv();

  constexpr view() = default;
  constexpr explicit view(lv_obj_t* p) : p_(p) {}

  /// Every widget is an obj.
  constexpr operator view<"obj">() const
    requires(W.sv() != "obj")
  {
    return view<"obj">{p_};
  }

  /// The LVGL widget: the one door to the C API.
  constexpr lv_obj_t* raw() const { return p_; }
  /// False for a view of nothing.
  constexpr explicit operator bool() const { return p_ != nullptr; }
  friend constexpr bool operator==(view, view) = default;

  /// Creates the widget as a child of `parent`. The parent owns it.
  static view create(view<"obj"> parent);

  /// The events of this widget, by name: co_await button.next().clicked.
  stub::events next() const;

  /// Creates a child of this widget: box.add<"label">().
  template <detail::fixed C>
  view<C> add() const;

  /// Applies typed properties, written in place: set({.value = 0}). A style in it is a named
  /// style, a look, or lv::style({...}); GCC refuses a style written out inline.
  view set(props<W>&& p) const;
  view set(const props<W>& p) const = delete(
      "lv: pass the properties in place: view.set({...}). A prepared set of properties is a "
      "function that calls set().");

  /// The untyped escape hatch: set<"text">("Speed") calls lv_label_set_text, or
  /// lv_obj_set_* as a fallback. The editor does not check the arguments.
  template <detail::fixed P, class... A>
  view set(A&&... a) const;

  /// get<"text">() calls lv_label_get_text, or lv_obj_get_* as a fallback. It returns
  /// what LVGL returns; an angle comes back as lv::angle or lv::decidegrees.
  template <detail::fixed P, class... A>
  typename detail::got<W, P, A...>::type get(A&&... a) const;

  /// Adds a handler: on<LV_EVENT_CLICKED>(handler). It takes lv::event or nothing, and
  /// captures nothing or one pointer-sized value.
  template <lv_event_code_t Code, event_handler F>
  view on(F) const { return *this; }
};

}  // namespace lv
