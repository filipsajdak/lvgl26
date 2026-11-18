#pragma once

// The enumerators that something made from an enum leaves out:
//   lv::options_of<quality, lv::but<quality::draft>>
//   lv::for_each<subpage, lv::but<subpage::none>>(part)
// It is in a header of its own, below lv/values.hpp: a list of options needs it, and lv/repeat.hpp
// includes the whole tree.

namespace lv {

template <auto... Values>
struct but {};

}  // namespace lv
