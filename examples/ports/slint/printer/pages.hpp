#pragma once

// The pages of the panel. Each page's file defines its lv::page, and the shell shows it with
// lv::shown: a page that is not shown has no widgets.

#include <lv/lv.hpp>
#include <lv/shown.hpp>

namespace printer {

//! [pages]
// The pages, each defined in its own file as lv::page_of<ui>: the shell shows them with
// lv::shown, and none of their trees is compiled in the shell.
extern const lv::page home_page;
extern const lv::page settings_page;
extern const lv::page ink_page;
extern const lv::page print_page;
extern const lv::page scan_page;
extern const lv::page copy_page;
extern const lv::page usb_page;
//! [pages]

}  // namespace printer
