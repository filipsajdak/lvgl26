#pragma once

// How a page is mounted and replaced. Each page's file defines one function, and the shell
// calls it when the page is shown: a page that is not shown has no widgets.

#include <lv/lv.hpp>

namespace printer {

// What a page's file gives the shell: the widgets of the page under `parent`, owned.
// Empty if LVGL has no memory.
using make_page = lv::owned<lv::obj> (*)(lv::obj parent);

lv::owned<lv::obj> mount_home(lv::obj parent);
lv::owned<lv::obj> mount_settings(lv::obj parent);
lv::owned<lv::obj> mount_ink(lv::obj parent);
lv::owned<lv::obj> mount_print(lv::obj parent);
lv::owned<lv::obj> mount_scan(lv::obj parent);
lv::owned<lv::obj> mount_copy(lv::obj parent);
lv::owned<lv::obj> mount_usb(lv::obj parent);

// Builds `Tree` under `parent` and hands its widgets over as one owner. Empty if LVGL has no memory.
template <const auto& Tree>
lv::owned<lv::obj> mount_page(lv::obj parent) {
  auto built = lv::mount<Tree>(parent);
  return built ? lv::owned<lv::obj>{built->detach()} : lv::owned<lv::obj>{};
}

}  // namespace printer
