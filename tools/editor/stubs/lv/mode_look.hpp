#pragma once

// Editor stub of lv/mode_look.hpp: a look whose styles follow the mode of a display.
//
//   template <> inline constexpr lv::mode_designs_of<day, dark> lv::mode_designs<lv::override>{};
//
//   lv::button({.look = lv::mode_look<"tile">})
//
// The editor does not check that both token files hold the group with one shape: GCC does.

#include <lv/detail/missing.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/look.hpp>
#include <lv/stub.hpp>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/mode_look.hpp"> = true;
}  // namespace lv::detail

namespace lv {

/// The tags of the designs of the modes, in the order of the enumerators of the state.
template <class... Designs>
struct mode_designs_of {};

namespace detail {
struct no_modes {};
}  // namespace detail

/// The designs of the modes: specialize it once, as lv::design<lv::override> is for lv::look.
template <class = override>
inline constexpr detail::no_modes mode_designs{};

/// The look of a group in the design of each mode, for `.look`: lv::mode_look<"tile">.
template <detail::fixed Paths, class D>
  requires detail::included<"lv/mode_look.hpp">
inline constexpr stub::mode_look_ref mode_look<Paths, D>{};

}  // namespace lv
