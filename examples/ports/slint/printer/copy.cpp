// The Copy page: how many copies, color or not, four choices, and a start button.

#include "fonts.hpp"
#include "pages.hpp"
#include "job_page.hpp"

namespace printer {
namespace {

//! [page]
constexpr page_fonts fonts{type(tok.text.title), type(tok.text.label), type(tok.text.body)};

constexpr auto ui =
  job_page<job_kind::copy>("Copy", "Size", "100%\n75%\n50%", "Start copying", fonts);

}  // namespace

constinit const lv::page copy_page = lv::page_of<ui>;
//! [page]

}  // namespace printer
