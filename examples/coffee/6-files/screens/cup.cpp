// The page where the reader sets the strength and the size of the cup, and its own states.

#include "cup.hpp"

#include "../design/design.hpp"
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

constexpr auto ml = mp_units::si::milli<mp_units::si::litre>;

//! [cup]
struct strength {
  int value;
};

constinit lv::state bold{strength{3}};
constinit lv::state cup{40 * ml};

constexpr auto least_dose = 6 * g;  // the beans of the mildest cup
constexpr auto dose_step  = 2 * g;  // what one step of strength adds

constinit lv::computed beans{[](strength s) { return least_dose + s.value * dose_step; }, bold};

constexpr auto strength_slider = lv::slider(
  {.range = {1, 5}, .value = lv::bind(bold), .width = 240 * px, .style = lv::look<"slider">}
);
constexpr auto size_slider = lv::slider(
  {.range = {20, 200}, .value = lv::bind(cup), .width = 240 * px, .style = lv::look<"slider">}
);
constexpr auto size_text =
  lv::label({.text = lv::bind(cup)});  // a quantity shows its unit: "40 mL"
constexpr auto beans_text = lv::label({.text = lv::bind(beans, "{} g of beans")});

constexpr auto your_cup = lv::group(
  {.style = lv::look<"screen">}, title("Your cup"), strength_slider, size_slider, size_text,
  beans_text, go_to<step::brewing>("Brew")
);
//! [cup]

}  // namespace

constinit const lv::page cup_page = lv::page_of<your_cup>;

}  // namespace coffee
