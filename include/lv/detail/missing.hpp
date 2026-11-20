#pragma once

// The templates and functions of lvgl26's opt-in headers, declared in the core so that a program
// that uses one without its header gets a compile error that names the header
// (docs/which-header.md):
//
//   error: static assertion failed: lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>
//
// Each opt-in header says that it was read by specializing `included` for its name, and declares
// its template as a specialization that requires that: a program that includes the header never
// reaches what is here. A function's fallback requires that the header was not read, so it never
// takes a call that the real function refuses with a message of its own. A class that is no
// template (lv::scaled_font, lv::typeface, lv::pointer_feed) cannot be declared twice, so it has
// no fallback; the build's check of the includes names its header (cmake/check_includes.cmake).
//
// Every opt-in header includes this file first: a program may include lv/list.hpp before
// lv/lv.hpp.

#include <lv/config.hpp>
#include <lv/detail/embedded.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/theme_styles.hpp>
#include <lv/tree.hpp>

#include <lvgl/lvgl.h>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace lv {

class display;

namespace detail {

// True once the opt-in header `Header` is read: the header specializes it.
template <fixed Header>
inline constexpr bool included = false;

// False, for a static_assert that fires only where its template is used.
template <class...>
inline constexpr bool header_missing = false;

// The constraints that the fallbacks and the real templates share: a constraint written twice is
// two constraints to the compiler, and the real template must be the more constrained one.
template <class... T>
concept tree_nodes = (tree_node<T> && ...);

// The header of each name declared below, for tools/reference/headers.cpp: reflection sees these
// declarations here, not the specializations in the headers. The program fails to compile for a
// name declared here and not listed.
struct fallback {
  std::string_view name;
  std::string_view header;
};
inline constexpr fallback fallbacks[] = {
    {"list"sv, "lv/list.hpp"sv},           {"each"sv, "lv/list.hpp"sv},
    {"for_each"sv, "lv/repeat.hpp"sv},     {"for_each_field"sv, "lv/repeat.hpp"sv},
    {"theme"sv, "lv/theme.hpp"sv},         {"look"sv, "lv/look.hpp"sv},
    {"image_file"sv, "lv/image.hpp"sv},    {"encoded_image"sv, "lv/encoded_image.hpp"sv},
    {"runtime"sv, "lv/runtime.hpp"sv},     {"one_of"sv, "lv/one_of.hpp"sv},
    {"pointer"sv, "lv/pointer.hpp"sv},     {"most_pointer_samples"sv, "lv/pointer.hpp"sv},
};

// What lv::theme's fallback takes: a type that no braced list of a theme converts to.
struct needs_theme_header {};

}  // namespace detail

// lv/list.hpp
template <class T, std::size_t N>
class list {
  static_assert(detail::header_missing<T>, "lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>");
};

template <class... A>
  requires(!detail::included<"lv/list.hpp">)
void each(A&&...) = delete("lv::each is in <lv/list.hpp>: add #include <lv/list.hpp>");

// lv/repeat.hpp
template <class E, class... X, class... A>
  requires(!detail::included<"lv/repeat.hpp">)
void for_each(A&&...) = delete("lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

template <const auto& Items, class... A>
  requires(!detail::included<"lv/repeat.hpp">)
void for_each(A&&...) = delete("lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

template <class T, class... A>
  requires(!detail::included<"lv/repeat.hpp">)
void for_each_field(A&&...) = delete("lv::for_each_field is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

// lv/theme.hpp
template <class D = void>
  requires(!detail::included<"lv/theme.hpp">)
theme_styles theme(detail::needs_theme_header) = delete("lv::theme is in <lv/theme.hpp>: add #include <lv/theme.hpp>");

// lv/look.hpp
template <detail::fixed Paths, class D = override>
inline constexpr auto look = [] {
  static_assert(detail::header_missing<D>, "lv::look is in <lv/look.hpp>: add #include <lv/look.hpp>");
  return 0;
}();

// lv/image.hpp
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* image_file = [] {
  static_assert(detail::header_missing<decltype(File)>, "lv::image_file is in <lv/image.hpp>: add #include <lv/image.hpp>");
  return static_cast<const lv_image_dsc_t*>(nullptr);
}();

// lv/encoded_image.hpp
template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* encoded_image = [] {
  static_assert(detail::header_missing<decltype(File)>,
                "lv::encoded_image is in <lv/encoded_image.hpp>: add #include <lv/encoded_image.hpp>");
  return static_cast<const lv_image_dsc_t*>(nullptr);
}();

// lv/runtime.hpp
template <detail::fixed... Widgets>
class runtime {
  static_assert(detail::header_missing<decltype(Widgets)...>,
                "lv::runtime is in <lv/runtime.hpp>: add #include <lv/runtime.hpp>");
};

// lv/one_of.hpp
template <const auto&... Trees>
  requires detail::tree_nodes<decltype(Trees)...>
class one_of {
  static_assert(detail::header_missing<decltype(Trees)...>, "lv::one_of is in <lv/one_of.hpp>: add #include <lv/one_of.hpp>");
};

// lv/pointer.hpp
inline constexpr std::size_t most_pointer_samples = 256;

namespace detail {
template <std::size_t Capacity>
concept pointer_capacity = Capacity <= most_pointer_samples;
}  // namespace detail

template <std::size_t Capacity = 16>
  requires detail::pointer_capacity<Capacity>
class pointer {
  static_assert(detail::header_missing<std::integral_constant<std::size_t, Capacity>>,
                "lv::pointer is in <lv/pointer.hpp>: add #include <lv/pointer.hpp>");

 public:
  explicit pointer(display&);
  pointer(display&&) = delete("the pointer keeps a pointer to its display: declare the display first, then the pointer");
};

}  // namespace lv
