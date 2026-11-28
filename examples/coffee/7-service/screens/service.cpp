// The page of the machine's settings, and its own states: the hardness of the water, which the
// machine's softener follows, and the cups left until the machine needs descaling.

#include "service.hpp"

#include "../design/design.hpp"
#include "../design/fonts.hpp"
#include "../parts/go_to.hpp"
#include "../parts/title.hpp"
#include "../state.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include <algorithm>
#include <utility>

namespace coffee {
namespace {

using namespace lv::units;

//! [service]
enum class hardness { soft, medium, hard };

constinit lv::state water{hardness::medium};

// Harder water scales the machine sooner: the cups between two descalings, in the order of the
// enum.
constexpr int cups_between[] = {200, 100, 50};

constinit lv::computed due{
  [](lv::counter made, hardness h) {
    return lv::counter{std::max(0, cups_between[std::to_underlying(h)] - made.value)};
  },
  cups,
  water
};
//! [service]

//! [effect]
hardness softener = hardness::medium;  // what the machine's softener is set to: outside the screen

// The softener follows the setting: once when the page is first shown, and after every change.
constinit lv::effect tell_softener{[](hardness h) { softener = h; }, water};
//! [effect]

//! [page]
// The list keeps LVGL's font: a drawn font has no symbol for its arrow.
constexpr auto water_menu = lv::dropdown({
  .options  = lv::options_of<hardness>,
  .selected = lv::bind(water),
  .width    = 240 * px,
});

constexpr auto made_text =
  lv::label({.text = lv::bind(cups, "{} cups since descaling"), .style = body_text});
constexpr auto due_text =
  lv::label({.text = lv::bind(due, "Descale in {} cups"), .style = lv::look<"price"> | body_text});

constexpr auto descaled =
  lv::button({.on_clicked = [] { cups = lv::counter{}; }}, lv::label({.text = "Descaled"}));

constexpr auto service = lv::group(
  {.style = lv::look<"screen">}, title("Service"), water_menu, made_text, due_text, descaled,
  go_to<step::drinks>("Done")
);

void follow_water(lv::held<service>) { tell_softener.start(); }
//! [page]

}  // namespace

constinit const lv::page service_page = lv::page_of<service, {.on_shown = &follow_water}>;

}  // namespace coffee
