// The page that brews the cup, and the task that brews it.

#include "brewing.hpp"

#include "../design/design.hpp"
#include "../design/fonts.hpp"
#include "../parts/go_to.hpp"
#include "../parts/title.hpp"
#include "../state.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include <mp-units/systems/si.h>

namespace coffee {
namespace {

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

//! [brewing]
constexpr auto brew_time = 25 * s;    // the time the machine brews, a value of the program
constexpr auto brew_span = 25 * sec;  // the same time, as LVGL counts it for the ring

constinit lv::state left{brew_time};

constexpr auto ring = lv::arc(
  {
    .id    = "ring",
    .range = {0, 100},
    .size  = {140 * px, 140 * px},
    .value = 0,
  },
  lv::label({.align = "center", .text = lv::bind(left), .style = body_text})  // shows "25 s"
);

constexpr auto brewing =
  lv::group({.style = lv::look<"screen">}, title("Brewing"), ring, go_to<step::cup>("Cancel"));

// The ring fills while the seconds count down. Both end when the brewing page goes: Cancel
// deletes the ring, which owns them.
lv::task<> fill(lv::view<"arc"> view) {
  co_await lv::animate(view, {.value = {0, 100}}, brew_span, *tok.motion.brew);
}

lv::task<> brew(lv::view<"arc"> view) {
  lv::spawn(fill(view), view);
  for (left = brew_time; left.get() > 0 * s; left -= 1 * s) co_await lv::sleep(1 * sec);
  current = step::enjoy;
}

void start_brewing(lv::held<brewing> page) {
  const auto view = page->get<"ring">();
  lv::spawn(brew(view), view);
}
//! [brewing]

}  // namespace

//! [page]
// The page's hook is named with the page, as an arm of lv::shown names it.
constinit const lv::page brewing_page = lv::page_of<brewing, {.on_shown = &start_brewing}>;
//! [page]

}  // namespace coffee
