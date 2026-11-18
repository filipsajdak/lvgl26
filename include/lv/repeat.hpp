#pragma once

// Repetition by reflection: the compiler writes the repeated widgets of a tree.
//
//   enum class subpage { none, print, scan, copy };
//
//   constexpr auto tile = []<subpage Page> {
//     return lv::button({.on_clicked = [] { active = Page; }})[
//         lv::label({.text = lv::name_of<Page>})];
//   };
//   constexpr auto home = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(tile)];
//
//   struct job {
//     [[=lv::caption{"queue.owner"}]] const char* owner;
//     [[=lv::caption{"queue.pages"}]] lv::counter pages;
//   };
//   constexpr auto details = lv::column[lv::for_each_field<job>([]<lv::field F> {
//     return lv::row[lv::label({.text = F.caption}), lv::label({.text = F.text()})];
//   })];
//
//   constexpr tube tubes[] = {{...}, {...}};
//   constexpr auto gauges = lv::row[lv::for_each<tubes>([]<tube T> { return gauge(T); })];
//
// The part is called once for each element, in the compiler, and what it returns becomes
// children of the widget that holds the lv::for_each: the tree is the one that would have
// been written by hand. Nothing is kept at run time, no list, no memory.
//
// What a part takes:
//
//   for_each<E>(part)       an enumerator of E as a template argument: []<E Value> { ... }
//   for_each_field<T>(part) a lv::field of T: []<lv::field F> { ... }. A member without
//                           a caption is not repeated.
//   for_each<items>(part)   one record of a constant array (a C array or std::array):
//                           []<record R> { ... } where the record is a structural type,
//                           []<const auto& R> { ... } where it is not.
//
// A part returns a widget. It sets no .id: the widgets of a repetition are not reached by
// id, and a repeated id would be used twice. For rows that change at run time use lv::each.

#include <lv/but.hpp>
#include <lv/list.hpp>
#include <lv/tree.hpp>

#include <array>
#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv {

// The text a member shows for itself, as an annotation: [[=lv::caption{"queue.owner"}]].
// It is a key of the strings, and a string literal: an annotation holds a structural type,
// which std::string_view is not.
template <std::size_t N>
struct caption {
  char key[N];

  consteval caption(const char (&text)[N]) {
    for (std::size_t i = 0; i < N; ++i) key[i] = text[i];
  }
  template <class T>
  consteval caption(const T&) = delete(
      "lv: a caption is a string literal: [[=lv::caption{\"queue.owner\"}]]. An annotation holds a "
      "structural type, so a std::string_view, a std::string or a const char* is no caption.");

  constexpr std::string_view sv() const { return {key, N - 1}; }
};
template <class T>
caption(const T&) -> caption<1>;

// One member of a row type: what lv::for_each_field gives the part.
//   []<lv::field F> { return lv::label({.text = F.caption}); }
// `member` is the pointer to the member, for lv::bind(F.member).
template <class C, class F>
struct field {
  F C::* member;
  const char* name;     // the member's name: "owner"
  const char* caption;  // its caption: "queue.owner"

  // The binding that shows the member as a text: lv::bind(member) for a text, a number
  // through "{}" for anything else.
  consteval auto text() const {
    if constexpr (detail::text_field<F>)
      return bind(member);
    else
      return bind(member, "{}");
  }
};

namespace detail {

// The failed expression is what the compiler prints for a part that captures something, so
// its name says what to do.
template <class F>
inline constexpr bool a_part_captures_nothing__name_what_it_needs_as_a_template_argument =
    std::is_class_v<std::remove_cvref_t<F>> && std::is_empty_v<std::remove_cvref_t<F>>;

}  // namespace detail

// What lv::for_each takes as its part: a generic lambda that captures nothing, as it runs in
// the compiler. What it needs, it names as a template argument or reads from a constant.
template <class F>
concept repeat_part = detail::a_part_captures_nothing__name_what_it_needs_as_a_template_argument<F>;

namespace detail {

template <class T>
inline constexpr bool is_but = false;
template <auto... V>
inline constexpr bool is_but<but<V...>> = true;

template <class E>
consteval std::string type_name() {
  return str(std::meta::identifier_of(^^E));
}

template <class E, auto V>
consteval bool is_enumerator() {
  if constexpr (!std::is_same_v<decltype(V), E>) {
    return false;
  } else {
    for (auto e : std::meta::enumerators_of(^^E))
      if (std::meta::extract<E>(e) == V) return true;
    return false;
  }
}

// The enumerators of E without those of Left, in the order they are declared.
template <class E, auto... Left>
consteval std::vector<E> enumerators_but() {
  static_assert(std::is_enum_v<E>,
                "lv: lv::for_each<E> takes an enum, one part for each of its enumerators. For a "
                "constant array write lv::for_each<items>(part), for the members of a struct "
                "lv::for_each_field<T>(part).");
  static_assert((is_enumerator<E, Left>() && ...),
                "lv: lv::but names a value that is no enumerator of the enum. Name enumerators "
                "of the enum given to lv::for_each: lv::for_each<subpage, lv::but<subpage::none>>.");
  std::vector<E> out;
  for (auto e : std::meta::enumerators_of(^^E)) {
    const E value = std::meta::extract<E>(e);
    if (((value != Left) && ...)) out.push_back(value);
  }
  if (out.empty())
    fail("lv: lv::for_each<" + type_name<E>() +
         "> has no enumerator left to repeat. Every enumerator is left out by lv::but, or the "
         "enum has none.");
  return out;
}

template <class E, class Left>
struct kept_values;
template <class E, auto... Left>
struct kept_values<E, but<Left...>> {
  static constexpr auto values = std::define_static_array(enumerators_but<E, Left...>());
};

// The enumerators that lv::for_each<E, Left> repeats.
template <class E, class Left>
inline constexpr auto kept_enumerators = kept_values<E, Left>::values;

consteval std::meta::info caption_of(std::meta::info member) {
  for (auto a : std::meta::annotations_of(member)) {
    const auto t = std::meta::type_of(a);
    if (std::meta::has_template_arguments(t) && std::meta::template_of(t) == ^^caption) return a;
  }
  return {};
}

// The members of T that have a caption, in the order they are declared.
template <class T>
consteval std::vector<std::meta::info> captioned_members() {
  static_assert(std::is_class_v<T>,
                "lv: lv::for_each_field<T> takes a struct whose members carry a caption: "
                "struct job { [[=lv::caption{\"queue.owner\"}]] const char* owner; };");
  std::vector<std::meta::info> out;
  for (auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))
    if (caption_of(m) != std::meta::info{}) out.push_back(m);
  if (out.empty())
    fail("lv: lv::for_each_field<" + type_name<T>() +
         "> finds no member with a caption, and a member without one is not repeated. Mark the "
         "members to show: [[=lv::caption{\"queue.owner\"}]] const char* owner;");
  return out;
}

template <class T>
inline constexpr auto captioned = std::define_static_array(captioned_members<T>());

// The lv::field of the member M of T.
template <class T, std::meta::info M>
consteval auto field_of() {
  constexpr auto written = [:std::meta::constant_of(caption_of(M)):];
  return field<T, typename[:std::meta::type_of(M):]>{&[:M:], std::define_static_string(std::meta::identifier_of(M)),
                                             std::define_static_string(written.sv())};
}

// A repeated widget sets no id.
template <class N>
consteval void check_no_ids(const N& n) {
  std::vector<id_entry> ids;
  collect(ids, n);
  if (!ids.empty())
    fail("lv: a part of lv::for_each sets the id '" + str(ids[0].name) +
         "', and a repetition would use it more than once. Leave the id out: the repeated widgets are "
         "not reached by id. For rows that change at run time use lv::each.");
}

template <class N>
consteval N checked_part(N n) {
  static_assert(tree_node<N>,
                "lv: the part of lv::for_each returns a widget: lv::button({...})[...], "
                "lv::row[...], lv::label({...}). It returned something else.");
  check_no_ids(n);
  return n;
}

// The part called for the value A, which a template argument gives it.
template <auto A, repeat_part F>
consteval auto part_for(const F& f) {
  static_assert(requires { f.template operator()<A>(); },
                "lv: the part of lv::for_each does not accept this element as a template argument. "
                "Write it as a generic lambda that takes it: []<subpage Page> { ... } for an "
                "enumerator, []<lv::field F> { ... } for a member, []<record R> { ... } for a record.");
  return checked_part(f.template operator()<A>());
}

// The same for a record that a constant array holds, by reference.
template <const auto& A, repeat_part F>
consteval auto part_for_record(const F& f) {
  static_assert(requires { f.template operator()<A>(); },
                "lv: the part of lv::for_each does not accept this record as a template argument. "
                "Write it as a generic lambda: []<record R> { ... } for a structural record, "
                "[]<const auto& R> { ... } for any other.");
  return checked_part(f.template operator()<A>());
}

template <class... Parts>
consteval auto repeat_of(Parts... parts) {
  return repeated<Parts...>{{parts...}};
}

template <class T>
concept constant_array = requires { std::size(std::declval<const T&>()); } && !std::is_enum_v<T>;

}  // namespace detail

// The enumerator V of an enum, as a text: name_of<subpage::print> is "print".
template <auto V>
inline constexpr const char* name_of = [] {
  using E = decltype(V);
  static_assert(std::is_enum_v<E>, "lv: lv::name_of<V> takes an enumerator.");
  for (auto e : std::meta::enumerators_of(^^E))
    if (std::meta::extract<E>(e) == V) return std::define_static_string(std::meta::identifier_of(e));
  detail::fail("lv: lv::name_of<V> takes an enumerator of " + detail::type_name<E>() + ", and V is none.");
}();

namespace detail {

// The member of Scope that has the name of the enumerator V.
template <auto V, class Scope>
consteval std::meta::info like_member() {
  const std::string_view wanted = name_of<V>;
  for (auto m : std::meta::members_of(^^Scope, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == wanted) return m;
  fail("lv: lv::like<" + type_name<decltype(V)>() + "::" + str(wanted) + ", " + type_name<Scope>() +
       "> finds no member named '" + str(wanted) + "' in " + type_name<Scope>() + ". Add one: a static "
       "constexpr member or an enumerator named '" + str(wanted) + "'.");
}

}  // namespace detail

// What the enumerator V names in Scope, a struct or an enum: with a scope
//   struct icons { static constexpr char print[] = "P"; ... };
// lv::like<subpage::print, icons> is icons::print. A function gives a pointer to it, an array
// a pointer to its first element. A scope without a member of that name is a compile error
// that names it.
template <auto V, class Scope>
inline constexpr auto like = [:detail::like_member<V, Scope>():];

// One part for each enumerator of E (without those in Left, if given), as children of the
// widget that holds it:
//   lv::row[lv::for_each<subpage, lv::but<subpage::none>>([]<subpage Page> { ... })]
template <class E, class Left = but<>, repeat_part F>
consteval auto for_each(const F& f) {
  static_assert(detail::is_but<Left>, "lv: what lv::for_each leaves out is lv::but<subpage::none>.");
  constexpr auto values = detail::kept_enumerators<E, Left>;
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::apply([](auto... parts) { return detail::repeat_of(parts...); },
                      std::make_tuple(detail::part_for<values[I]>(f)...));
  }(std::make_index_sequence<values.size()>{});
}

// One part for each captioned member of T, as children of the widget that holds it:
//   lv::column[lv::for_each_field<job>([]<lv::field F> { ... })]
template <class T, repeat_part F>
consteval auto for_each_field(const F& f) {
  constexpr auto members = detail::captioned<T>;
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::apply([](auto... parts) { return detail::repeat_of(parts...); },
                      std::make_tuple(detail::part_for<detail::field_of<T, members[I]>()>(f)...));
  }(std::make_index_sequence<members.size()>{});
}

// One part for each record of the constant array `items`, as children of the widget that
// holds it:
//   constexpr tube tubes[] = {{...}, {...}};
//   lv::row[lv::for_each<tubes>([]<tube T> { ... })]
template <const auto& items, repeat_part F>
  requires detail::constant_array<std::remove_cvref_t<decltype(items)>>
consteval auto for_each(const F& f) {
  static_assert(std::size(items) > 0, "lv: lv::for_each<items> has no record to repeat: the array is empty.");
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::apply([](auto... parts) { return detail::repeat_of(parts...); },
                      std::make_tuple(detail::part_for_record<items[I]>(f)...));
  }(std::make_index_sequence<std::size(items)>{});
}

}  // namespace lv
