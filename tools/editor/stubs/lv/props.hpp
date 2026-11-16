#pragma once

// Editor stub of lv/props.hpp.
//
//   lv::props<"arc"> p{.id = "speedo", .range = {0, 240}, .rotation = 90 * lv::deg, .value = 120};
//
// The structs are in lv/generated.hpp: lv::stub::props_arc and the others.

#include <lv/generated.hpp>
#include <lv/style.hpp>
#include <lv/values.hpp>

namespace lv {

namespace detail {

/// A widget LVGL has: "arc", "label", "obj", ... The reference lists them.
template <fixed W>
concept known_widget = requires { typename stub::props_for<W>::type; };

}  // namespace detail

/// The properties of one kind of widget: one member for each LVGL setter, one for
/// each event (on_clicked), an `id`, and `style` with its parts and states.
template <detail::fixed W>
  requires detail::known_widget<W>
using props = typename stub::props_for<W>::type;

}  // namespace lv
