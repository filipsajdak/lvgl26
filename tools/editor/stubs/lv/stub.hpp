#pragma once

// What the generated stub headers fill in: lv/generated.hpp and lv/token_files.hpp
// specialize these templates. Only the stubs have this header.

#include <lv/detail/fixed_string.hpp>

#include <cstddef>
#include <cstdint>

namespace lv::stub {

// A function of a template has a body here, also when nothing is known about what it
// does: its arguments may be types without linkage (a lambda, a tree in an unnamed
// namespace), and Clang warns about such a function when it is used and not defined.
// The body is then `__builtin_unreachable()`, which fits every return type.

/// props_for<"arc">::type is the struct of the properties of an arc. A widget that
/// LVGL does not have has no such type.
template <detail::fixed W>
struct props_for;

/// animatable_for<"arc">::type lists the properties of an arc that lv::animate can run.
template <detail::fixed W>
struct animatable_for;

/// getter<"arc", "value">::call(o) has the type of what lv_arc_get_value returns.
template <detail::fixed W, detail::fixed P>
struct getter;

/// Unit, when Raw is a type: the result of a getter that returns an angle.
template <class Unit, class Raw>
using unit_of = Unit;

/// What `.look` takes: the value of lv::mode_look<"tile">.
struct mode_look_ref {};

/// Stands for a value whose type the stubs do not know. It takes and gives anything,
/// so the editor marks nothing there; GCC checks the value when the program is built.
struct anything {
  constexpr anything() = default;
  template <class... A>
  constexpr anything(A&&...) {}
  template <class T>
  constexpr operator T() const;
};

/// The tokens of a file that the stub generator has not seen. Run CMake again, or
/// name the file: lvgl26_editor_support(TOKENS file.json).
struct unknown_token_file {};

/// The groups of a token file that lv::look can name, as one text of dotted paths with a space
/// between them: tools/editor/generate.py writes one for each token struct. Empty for a struct it
/// has not seen: nothing is checked then.
template <class Tokens>
inline constexpr const char* look_groups = "";

/// token_file<key>::type is the struct of the token file with that key.
template <std::uint64_t Key>
struct token_file {
  using type = unknown_token_file;
};

}  // namespace lv::stub
