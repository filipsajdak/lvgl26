// The Scan page: how many scans, color or not, four choices, and a start button.

#include "fonts.hpp"
#include "pages.hpp"
#include "job_page.hpp"

namespace printer {
namespace {

constexpr page_fonts fonts{type(tok.text.title), type(tok.text.label), type(tok.text.body)};

constexpr auto ui = job_page<job_kind::scan>("Scan", "Format", "PDF\nJPEG\nPNG", "Start scanning", fonts);

}  // namespace

lv::owned<lv::obj> mount_scan(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
