#pragma once

// What the pages read and write. Each is one object for the whole program: a page's file
// binds to it, and the shell follows it.

#include <lv/lv.hpp>

namespace printer {

//! [state]
enum class page { home, settings, ink };
enum class subpage { none, print, scan, copy, usb };

inline constinit lv::state<page> active_page{page::home};
inline constinit lv::state<subpage> active_subpage{subpage::none};

inline void open_print() { active_subpage = subpage::print; }
inline void open_scan() { active_subpage = subpage::scan; }
inline void open_copy() { active_subpage = subpage::copy; }
inline void open_usb() { active_subpage = subpage::usb; }
inline void close_subpage() { active_subpage = subpage::none; }
//! [state]

// A number the user can raise and lower between 1 and 100, as Copies and Scans.
//! [counter]
constexpr lv::counter one_fewer(lv::counter n) { return n.value > 1 ? lv::counter{n.value - 1} : n; }
constexpr lv::counter one_more(lv::counter n) { return n.value < 100 ? lv::counter{n.value + 1} : n; }
//! [counter]

// The choice between color and grayscale of a job, and its name on the button.
//! [color_name]
constexpr const char* color_name(bool color) { return color ? "Color" : "Grayscale"; }
//! [color_name]

}  // namespace printer
