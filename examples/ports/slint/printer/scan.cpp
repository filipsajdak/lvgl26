// The Scan page: how many scans, color or not, four choices, and a start button.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

namespace printer {
namespace {

constinit lv::state scans{lv::counter{1}};
constinit lv::state<bool> in_color{true};
constinit lv::computed color_label{&color_name, in_color};

void fewer() { scans.update(&one_fewer); }
void more() { scans.update(&one_more); }
void toggle_color() { in_color = !in_color.get(); }
void start() {
  start_job("Scan");
  close_subpage();
}

constexpr page_fonts fonts{type(tok.text.title), type(tok.text.label), type(tok.text.body)};

constexpr auto ui = job_page("Scan", scans, color_label, {&fewer, &more, &toggle_color, &start}, "Format",
                             "PDF\nJPEG\nPNG", "Start scanning", fonts);

}  // namespace

lv::owned<lv::obj> mount_scan(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
