// The Copy page: how many copies, color or not, four choices, and a start button.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"
#include "queue.hpp"

namespace printer {
namespace {

constinit lv::state copies{lv::counter{1}};
constinit lv::state<bool> in_color{true};
constinit lv::computed color_label{&color_name, in_color};

void fewer() { copies.update(&one_fewer); }
void more() { copies.update(&one_more); }
void toggle_color() { in_color = !in_color.get(); }
void start() {
  start_job("Copy");
  close_subpage();
}

constexpr page_fonts fonts{type(tok.text.title), type(tok.text.label), type(tok.text.body)};

constexpr auto ui = job_page("Copy", copies, color_label, {&fewer, &more, &toggle_color, &start}, "Size",
                             "100%\n75%\n50%", "Start copying", fonts);

}  // namespace

lv::owned<lv::obj> mount_copy(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
