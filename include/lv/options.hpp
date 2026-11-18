#pragma once

// Lists of options, for a dropdown and a roller.
//
//   lv::dropdown({.options = {"100%", "75%", "50%"}})        // joined by the compiler
//   lv::dropdown({.options = lv::options_of<quality>})       // one option per enumerator
//   lv::dropdown({.options = lv::options_of<quality, lv::but<quality::draft>>,
//                 .selected = lv::bind(chosen)})            // chosen holds a quality
//   lv::roller({.options = {{"Mon", "Tue", "Wed"}, "infinite"}})  // the mode follows the list
//
// LVGL takes the options as one text, separated by line breaks. The compiler joins the
// list into that text and keeps it in read-only memory: a dropdown then keeps the
// pointer (lv_dropdown_set_options_static) and copies nothing. A roller always copies
// the text into its label: LVGL has no static setter for it.
//
// Which properties take such a list is found by the name of the setter, and tests/options_docs.cpp
// checks the names against LVGL's doc comments (lv/detail/option_docs.hpp).
//
// A state of the enum follows `.selected` in both directions when the options come from
// lv::options_of<E>: the user's choice sets the enumerator, and the state shows in the
// widget. The mapping between an enumerator and its place in the list is made at compile
// time, and it respects lv::but.
//
// The text of an option is the name of its enumerator. To give another text, specialize
// lv::option_text for the enum:
//
//   template <>
//   struct lv::option_text<quality> {
//     static consteval const char* of(quality q) { ... }
//   };

#include <lv/but.hpp>
#include <lv/detail/api.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace lv {

namespace detail {

// What makes an option_list, written out so that lv::option_list stays one plain struct
// of pointers.
struct list_parts {
  const char* text;
  const char* enum_name;
  const std::int32_t* values;
  std::uint32_t count;
  bool kept;
};

// The text in quotes, a line break written as \n.
consteval std::string quoted(std::string_view text) {
  std::string out = "\"";
  for (const char c : text) out.append(c == '\n' ? std::string_view{"\\n"} : std::string_view{&c, 1});
  return out.append("\"");
}

// Stops the compilation when option number `number` (counted from 1) cannot be one.
consteval void check_option(std::string_view option, std::size_t number) {
  if (option.empty())
    fail(join({"lv: option ", decimal(number),
               " of the list is empty. LVGL takes the options as one text separated by line "
               "breaks, and an empty one would show as a blank row."}));
  for (const char c : option)
    if (c == '\n')
      fail(join({"lv: option ", decimal(number), " of the list, ", quoted(option),
                 ", holds a line break. The list is the separator: write each option as an item, "
                 ".options = {\"100%\", \"75%\", \"50%\"}, not \"100%\\n75%\\n50%\"."}));
}

// The options as the one text LVGL takes, in static storage.
consteval const char* joined_text(std::initializer_list<std::string_view> items) {
  std::string joined;
  std::size_t number = 0;
  for (const std::string_view item : items) {
    check_option(item, ++number);
    if (number > 1) joined += '\n';
    joined += item;
  }
  return std::define_static_string(joined);
}

}  // namespace detail

// A list of options: the text LVGL takes, and what a state of an enum needs to follow it.
//
// It is made by `.options = {"a", "b", "c"}` and by lv::options_of<E>. A plain text
// ("a\nb\nc") converts to one as well, and is copied by LVGL, as before.
// It has no default member initializer, on purpose: see lv::arg.
struct option_list {
  const char* text;                // the options, separated by '\n'
  const char* enum_name;           // the enum the options come from, or null
  const std::int32_t* values;      // with an enum: the value of the enumerator of each option
  std::uint32_t count;             // with an enum: the number of options
  bool kept;                       // text is in read-only memory, so LVGL may keep the pointer

  constexpr option_list() = default;

  // A text of options separated by line breaks. It may be a temporary: LVGL copies it,
  // except where a constexpr tree holds it.
  constexpr option_list(const char* options) : text(options), enum_name(nullptr), values(nullptr), count(0), kept(false) {}

  // Two or more options. Not a single one: `{"a"}` is the text "a".
  template <std::size_t... N>
    requires(sizeof...(N) >= 2)
  consteval option_list(const char (&... item)[N])
      : text(detail::joined_text({std::string_view{item}...})),
        enum_name(nullptr),
        values(nullptr),
        count(0),
        kept(true) {}

  explicit constexpr option_list(detail::list_parts p)
      : text(p.text), enum_name(p.enum_name), values(p.values), count(p.count), kept(p.kept) {}

  constexpr operator const char*() const { return text; }

  // The place in the list of the enumerator whose value is `value`, or `count`.
  constexpr std::uint32_t index_of(std::int32_t value) const {
    std::uint32_t i = 0;
    while (i < count && values[i] != value) ++i;
    return i;
  }
};

// The text of an option made from an enumerator. The primary template gives its name;
// specialize it to give another text (a later strings table plugs in here):
//   template <> struct lv::option_text<quality> {
//     static consteval const char* of(quality q) { ... }
//   };
template <class E>
struct option_text {
  static_assert(std::is_enum_v<E>, "lv: lv::option_text<E> is for an enum.");

  static consteval std::string_view of(E value) {
    for (const auto e : std::meta::enumerators_of(^^E))
      if (std::meta::extract<E>(e) == value) return std::meta::identifier_of(e);
    return {};
  }
};

namespace detail {

template <class E>
consteval std::string type_text() {
  return str(std::meta::display_string_of(std::meta::dealias(^^E)));
}

// The list of options that were collected, each text followed by a line break.
consteval option_list options_listed(const std::string& type, std::string& joined, const std::vector<std::int32_t>& values) {
  if (values.empty())
    fail(join({"lv: lv::options_of<", type,
               "> has no option left. Every enumerator is left out by lv::but, or the enum has none."}));
  joined.pop_back();
  return option_list{list_parts{std::define_static_string(joined), std::define_static_string(type),
                                std::define_static_array(values).data(),
                                static_cast<std::uint32_t>(values.size()), true}};
}

// The options of E without the enumerators of Left, in the order they are declared.
template <class E, auto... Left>
consteval option_list options_without(but<Left...>) {
  static_assert(std::is_enum_v<E>,
                "lv: lv::options_of<E> takes an enum, one option for each of its enumerators.");
  static_assert((std::same_as<decltype(Left), E> && ...),
                "lv: lv::but in lv::options_of names enumerators of the enum given to "
                "lv::options_of: lv::options_of<quality, lv::but<quality::draft>>.");
  static_assert(sizeof(E) <= sizeof(std::int32_t),
                "lv: an enum that lv::options_of lists is at most 32 bits wide, which is what a state keeps.");
  if constexpr (std::is_enum_v<E> && (std::same_as<decltype(Left), E> && ...)) {
    std::string joined;
    std::vector<std::int32_t> values;
    for (const auto e : std::meta::enumerators_of(^^E)) {
      const E value = std::meta::extract<E>(e);
      if (((value == Left) || ...)) continue;
      const std::string_view text{option_text<E>::of(value)};
      check_option(text, values.size() + 1);
      joined.append(text).append("\n");
      values.push_back(static_cast<std::int32_t>(value));
    }
    return options_listed(type_text<E>(), joined, values);
  } else {
    return {};
  }
}

}  // namespace detail

// One option for each enumerator of E, in the order they are declared, except those of
// Left. `.selected = lv::bind(state)` of a state that holds an E follows the choice.
//   lv::options_of<quality>
//   lv::options_of<quality, lv::but<quality::draft>>
template <class E, class Left = but<>>
inline constexpr option_list options_of = detail::options_without<E>(Left{});

}  // namespace lv
