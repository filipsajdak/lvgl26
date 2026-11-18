#pragma once

// Editor stub of lv/but.hpp: the enumerators that something made from an enum leaves out.

namespace lv {

/// The enumerators left out: `lv::options_of<quality, lv::but<quality::draft>>` and
/// `lv::for_each<subpage, lv::but<subpage::none>>(part)`.
template <auto... Values>
struct but {};

}  // namespace lv
