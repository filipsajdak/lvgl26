#pragma once

// Editor stub of lv/detail/missing.hpp: the templates and functions of the opt-in headers,
// declared in the core, so that clangd names the header in the diagnostic of a file that uses
// one without it. A function's fallback returns stub::anything, so that the deleted call is the one
// error on its line. As in lvgl26, each opt-in stub specializes `included` and declares its
// template as a specialization that requires it.

#include <lv/detail/embedded.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/theme_styles.hpp>
#include <lv/match.hpp>
#include <lv/generated.hpp>
#include <lv/props.hpp>
#include <lv/stub.hpp>
#include <lv/tree.hpp>

#include <lvgl/lvgl.h>

#include <cstddef>

namespace lv {

class display;

namespace detail {

template <fixed Header>
inline constexpr bool included = false;

template <class...>
inline constexpr bool header_missing = false;

// The condition of a function's fallback. It names the call's own template parameters, so that it is
// checked at the call: a condition that depends on nothing is worked out where the fallback is
// declared, before the header could say it was read, and Clang refuses the header's specialization.
template <fixed Header, class... Call>
concept not_read = !included<Header>;

template <class... T>
concept tree_nodes = (tree_node<T> && ...);

template <fixed... Widgets>
concept known_widgets = (known_widget<Widgets> && ...);

}  // namespace detail

template <class T, std::size_t N>
class list {
  static_assert(detail::header_missing<T>, "lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>");
};

template <class... A>
  requires detail::not_read<"lv/list.hpp", A...>
stub::anything each(A&&...) = delete("lv::each is in <lv/list.hpp>: add #include <lv/list.hpp>");

template <class E, class... X, class... A>
  requires detail::not_read<"lv/repeat.hpp", A...>
stub::anything for_each(A&&...) = delete("lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

template <const auto& Items, class... A>
  requires detail::not_read<"lv/repeat.hpp", A...>
stub::anything for_each(A&&...) = delete("lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

template <class T, class... A>
  requires detail::not_read<"lv/repeat.hpp", A...>
stub::anything for_each_field(A&&...) = delete("lv::for_each_field is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>");

template <class D = void>
  requires detail::not_read<"lv/theme.hpp", D>
theme_styles theme(const stub::theme_spec&) = delete("lv::theme is in <lv/theme.hpp>: add #include <lv/theme.hpp>");

template <detail::fixed Paths, class D = override>
inline constexpr auto look = [] {
  static_assert(detail::header_missing<D>, "lv::look is in <lv/look.hpp>: add #include <lv/look.hpp>");
  return 0;
}();

template <detail::fixed Paths, class D = override>
inline constexpr auto mode_look = [] {
  static_assert(detail::header_missing<D>, "lv::mode_look is in <lv/mode_look.hpp>: add #include <lv/mode_look.hpp>");
  return 0;
}();

template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* image_file = [] {
  static_assert(detail::header_missing<decltype(File)>, "lv::image_file is in <lv/image.hpp>: add #include <lv/image.hpp>");
  return static_cast<const lv_image_dsc_t*>(nullptr);
}();

template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* mask_file = [] {
  static_assert(detail::header_missing<decltype(File)>, "lv::mask_file is in <lv/image.hpp>: add #include <lv/image.hpp>");
  return static_cast<const lv_image_dsc_t*>(nullptr);
}();

template <const auto& File>
  requires detail::embedded_bytes<decltype(File)>
inline constexpr const lv_image_dsc_t* encoded_image = [] {
  static_assert(detail::header_missing<decltype(File)>,
                "lv::encoded_image is in <lv/encoded_image.hpp>: add #include <lv/encoded_image.hpp>");
  return static_cast<const lv_image_dsc_t*>(nullptr);
}();

template <detail::fixed... Widgets>
  requires detail::known_widgets<Widgets...>
class runtime {
  static_assert(detail::header_missing<decltype(Widgets)...>,
                "lv::runtime is in <lv/runtime.hpp>: add #include <lv/runtime.hpp>");
};

template <const auto&... Trees>
  requires detail::tree_nodes<decltype(Trees)...>
class one_of {
  static_assert(detail::header_missing<decltype(Trees)...>, "lv::one_of is in <lv/one_of.hpp>: add #include <lv/one_of.hpp>");
};

/// The largest queue of a pointer.
inline constexpr std::size_t most_pointer_samples = 256;

namespace detail {
template <std::size_t Capacity>
concept pointer_capacity = Capacity <= most_pointer_samples;
}  // namespace detail

template <std::size_t Capacity = 16>
  requires detail::pointer_capacity<Capacity>
class pointer {
  static_assert(detail::header_missing<decltype(Capacity)>, "lv::pointer is in <lv/pointer.hpp>: add #include <lv/pointer.hpp>");

 public:
  explicit pointer(display&) {}
};

// lv/shown.hpp
namespace detail {
template <class Pattern, class T>
struct tree_arm {
  static_assert(header_missing<T>, "lv::is<...>(tree) is an arm of lv::shown, in <lv/shown.hpp>: add #include <lv/shown.hpp>");
};
}  // namespace detail

template <class... A>
  requires detail::not_read<"lv/shown.hpp", A...>
stub::anything shown(A&&...) = delete("lv::shown is in <lv/shown.hpp>: add #include <lv/shown.hpp>");

template <class... A>
  requires detail::not_read<"lv/shown.hpp", A...>
stub::anything when(A&&...) = delete("lv::when is in <lv/shown.hpp>: add #include <lv/shown.hpp>");

}  // namespace lv
