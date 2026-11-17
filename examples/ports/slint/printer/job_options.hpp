#pragma once

// What the Copy, Scan and USB pages share: how many, color or not, and the functions that change
// them. One state for each kind of page, which keeps its value while the page is away.

#include "queue.hpp"
#include "state.hpp"

namespace printer {

enum class job_kind { copy, scan, usb };

constexpr const char* job_name(job_kind kind) {
  return kind == job_kind::copy ? "Copy" : kind == job_kind::scan ? "Scan" : "USB";
}

template <job_kind Kind>
struct job_options {
//! [color]
  static inline constinit lv::state<lv::counter> count{lv::counter{1}};
  static inline constinit lv::state<bool> in_color{true};
  static inline constinit lv::computed color_label{&color_name, in_color};
//! [color]

//! [actions]
  static void fewer() { count.update(&one_fewer); }
  static void more() { count.update(&one_more); }
  static void toggle_color() { in_color = !in_color.get(); }
  static void start() {
    start_job(job_name(Kind));
    close_subpage();
  }
//! [actions]
};

}  // namespace printer
