#pragma once

// A place on the screen where one page at a time is mounted. Showing another page deletes
// the widgets of the one before. A slide pager moves the page in from the right and out again.

#include "pages.hpp"
#include "ui.hpp"

namespace printer {

//! [pager]
// Replaces the page in `slot` with the one `make` builds under `host`.
// The page before is deleted first, so two pages never exist at once.
inline void replace_page(lv::owned<lv::obj>& slot, lv::obj host, make_page make) {
  slot.reset();
  slot = make(host);
}

class pager {
 public:
  void host(lv::obj where) { host_ = where; }
  void show(make_page make) { replace_page(current_, host_, make); }
  void clear() { current_.reset(); }

 protected:
  lv::obj host_;
  lv::owned<lv::obj> current_;
};
//! [pager]

// How a page leaves: it slides to the right, and its widgets go when it is out of sight.
inline lv::task<> slide_out(lv::owned<lv::obj> page) {
  co_await lv::animate(page.get(), {.x = {0 * px, page_width * px}}, tok.motion.slide_time, tok.motion.slide);
}

inline lv::task<> slide_in(lv::obj page) {
  co_await lv::animate(page, {.x = {page_width * px, 0 * px}}, tok.motion.slide_time, tok.motion.slide);
}

// A pager whose pages slide.
class slide_pager : public pager {
 public:
  void show(make_page make) {
    leave();
    replace_page(current_, host_, make);
    if (current_) lv::spawn(slide_in(current_.get()), current_.get());
  }

  // The page slides out, and the one that takes its place does not wait for it.
  void leave() {
    if (!current_) return;
    lv::spawn(slide_out(std::move(current_)));
    current_ = {};
  }
};

}  // namespace printer
