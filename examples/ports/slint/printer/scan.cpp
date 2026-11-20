// The Scan page: how many scans, color or not, four choices, and a start button.

#include "fonts.hpp"
#include "pages.hpp"
#include "job_page.hpp"

namespace printer {
namespace {

constexpr page_fonts fonts{type(tok.text.title), type(tok.text.label), type(tok.text.body)};

constexpr auto ui = job_page<job_kind::scan>("Scan", "Format", "PDF\nJPEG\nPNG", "Start scanning", fonts);

}  // namespace

constinit const lv::page scan_page = lv::page_of<ui>;

}  // namespace printer
