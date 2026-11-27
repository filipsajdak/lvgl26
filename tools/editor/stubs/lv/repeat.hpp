#pragma once

// Editor stub of lv/repeat.hpp: repetition by reflection.
//
//   constexpr auto tile = []<subpage Page> { return lv::label({.text = lv::name_of<Page>}); };
//   constexpr auto home = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(tile)];
//
// The editor does not run the part: GCC does, and checks every element.

#include <lv/detail/missing.hpp>
#include <lv/but.hpp>
#include <lv/tree.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/repeat.hpp"> = true;
}  // namespace lv::detail

namespace lv {

namespace detail {

template <class F>
inline constexpr bool a_part_captures_nothing__name_what_it_needs_as_a_template_argument =
    std::is_class_v<std::remove_cvref_t<F>> && std::is_empty_v<std::remove_cvref_t<F>>;

}  // namespace detail

/// What lv::for_each takes as its part: a generic lambda that captures nothing.
template <class F>
concept repeat_part = detail::a_part_captures_nothing__name_what_it_needs_as_a_template_argument<F>;

/// The text a member shows for itself, as an annotation: [[=lv::caption{"queue.owner"}]].
template <std::size_t N>
struct caption {
  char key[N];
  consteval caption(const char (&text)[N]) {
    for (std::size_t i = 0; i < N; ++i) key[i] = text[i];
  }
  constexpr std::string_view sv() const { return {key, N - 1}; }
};

/// One member of a row type: what lv::for_each_field gives the part.
///   []<lv::field F> { return lv::label({.text = F.caption}); }
template <class C, class F>
struct field {
  /// The pointer to the member, for lv::bind(F.member).
  F C::* member;
  /// The member's name.
  const char* name;
  /// Its caption: a key of the strings.
  const char* caption;

  /// The binding that shows the member as a text.
  consteval stub::anything text() const { return {}; }
};

namespace detail {

/// The widgets that a repetition makes: the parent's [] takes them as its children.
struct repeated_stub {};

template <>
inline constexpr bool is_structural<repeated_stub> = true;

}  // namespace detail

/// The enumerator V of an enum, as a text: name_of<subpage::print> is "print".
/// The editor cannot read the name, so the stub is an empty text: still a constant, as an id or a
/// text made from it must be.
template <auto V>
inline constexpr const char* name_of = "";

namespace stub {

template <class T>
inline constexpr bool is_property_holder = false;
template <class... Ts>
inline constexpr bool is_property_holder<lv::arg<Ts...>> = true;

/// What lv::like names, which the editor cannot find: a value of the type of any property. It
/// does not convert to a property's holder (lv::arg), so `.text = lv::like<...>` is read one way.
struct member_of_scope {
  template <class T>
    requires(!is_property_holder<std::remove_cvref_t<T>>)
  constexpr operator T() const {
    return T{};
  }
};

}  // namespace stub

/// What the enumerator V names in Scope, a struct or an enum: lv::like<subpage::print, icons>.
template <auto V, class Scope>
inline constexpr stub::member_of_scope like{};

/// One part for each enumerator of E (without those in Left, if given).
template <class E, class Left = but<>, repeat_part F>
constexpr detail::repeated_stub for_each(const F& part) {
  return {};
}

/// One part for each captioned member of T.
template <class T, repeat_part F>
constexpr detail::repeated_stub for_each_field(const F& part) {
  return {};
}

/// One part for each record of the constant array `items`.
template <const auto& items, repeat_part F>
constexpr detail::repeated_stub for_each(const F& part) {
  return {};
}

}  // namespace lv
