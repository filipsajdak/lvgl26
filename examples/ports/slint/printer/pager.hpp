#pragma once

// A place on the screen where one page at a time is mounted. Showing another page deletes
// the widgets of the one before. A slide pager moves the page in from the right and out again.

#include "pages.hpp"
#include "ui.hpp"

namespace printer {

//! [pager]
class pager {
 public:
  void host(lv::obj where) { host_ = where; }

  // Replaces the page that is shown with the one `make` builds.
  void show(make_page make) {
    current_.reset();
    current_ = make(host_);
  }

  // Deletes the page.
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
    current_ = make(host_);
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
