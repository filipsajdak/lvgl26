#pragma once

// How a page is mounted. Each page's file defines one function, and the shell calls it when the
// page is shown: a page that is not shown has no widgets.

#include <lv/lv.hpp>

namespace treadmill {

// What a page's file gives the shell: the widgets of the page under `parent`, owned.
// Empty if LVGL has no memory.
using make_page = lv::owned<lv::obj> (*)(lv::obj parent);

lv::owned<lv::obj> mount_splash(lv::obj parent);
lv::owned<lv::obj> mount_main_light(lv::obj parent);
lv::owned<lv::obj> mount_main_dark(lv::obj parent);
lv::owned<lv::obj> mount_main_sporty(lv::obj parent);

// Builds `Tree` under `parent` and hands its widgets over as one owner. Empty if LVGL has no memory.
template <const auto& Tree>
lv::owned<lv::obj> mount_page(lv::obj parent) {
  auto built = lv::mount<Tree>(parent);
  return built ? lv::owned<lv::obj>{built->detach()} : lv::owned<lv::obj>{};
}

}  // namespace treadmill
