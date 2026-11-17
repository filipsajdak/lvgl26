#pragma once

// Editor stub of lv/repeat.hpp: repetition by reflection.
//
//   constexpr auto tile = []<subpage Page> { return lv::label({.text = lv::name_of<Page>}); };
//   constexpr auto home = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(tile)];
//
// The editor does not run the part: GCC does, and checks every element.

#include <lv/tree.hpp>

#include <cstddef>
#include <string_view>

namespace lv {

/// The enumerators that lv::for_each leaves out: lv::for_each<subpage, lv::but<subpage::none>>.
template <auto... Values>
struct but {};

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
template <auto V>
inline constexpr const char* name_of = nullptr;

/// What the enumerator V names in Scope, a struct or an enum: lv::like<subpage::print, icons>.
template <auto V, class Scope>
inline constexpr stub::anything like{};

/// One part for each enumerator of E (without those in Left, if given).
template <class E, class Left = but<>, class F>
constexpr detail::repeated_stub for_each(const F& part) {
  return {};
}

/// One part for each captioned member of T.
template <class T, class F>
constexpr detail::repeated_stub for_each_field(const F& part) {
  return {};
}

/// One part for each record of the constant array `items`.
template <const auto& items, class F>
constexpr detail::repeated_stub for_each(const F& part) {
  return {};
}

}  // namespace lv
