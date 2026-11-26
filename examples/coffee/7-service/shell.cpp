// The frame of the machine: it shows the page of the current step, and a page slides in when the
// step changes. The pages are defined in their own files, so none of their trees is compiled here.

#include "shell.hpp"

#include "screens/brewing.hpp"
#include "screens/cup.hpp"
#include "screens/drinks.hpp"
#include "screens/enjoy.hpp"
#include "screens/service.hpp"
#include "state.hpp"

#include <lv/lv.hpp>
#include <lv/shown.hpp>

namespace coffee {
namespace {

using namespace lv::units;

//! [pages]
// A page slides in from the right, and the old one slides out to the left.
constexpr auto slide_in  = lv::transition({.x = {width * px, 0 * px}}, 250 * lv::ms, lv::ease::out);
constexpr auto slide_out = lv::transition({.x = {0 * px, -width * px}}, 250 * lv::ms, lv::ease::in);

constexpr auto ui = lv::group(
  lv::shown(
    current, {.enter = slide_in, .leave = slide_out}, lv::is<step::drinks>(drinks_page),
    lv::is<step::cup>(cup_page), lv::is<step::brewing>(brewing_page),
    lv::is<step::enjoy>(enjoy_page), lv::is<step::service>(service_page)
  )
);
//! [pages]

}  // namespace

// The frame lives as long as the program. Its pages come from LVGL's heap: one block for two of
// them, since the shell cannot see their size.
bool start(lv::obj screen) {
  static auto frame = lv::mount<ui>(screen);
  return static_cast<bool>(frame);
}

}  // namespace coffee
