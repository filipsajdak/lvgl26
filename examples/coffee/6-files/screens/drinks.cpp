// The page where the reader picks a drink.

#include "drinks.hpp"

#include "../design/design.hpp"
#include "../parts/go_to.hpp"
#include "../parts/title.hpp"
#include "../state.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/shown.hpp>

namespace coffee {
namespace {

using namespace lv::units;

//! [drinks]
constexpr auto menu = lv::dropdown({
  .options  = lv::options_of<drink>,
  .selected = lv::bind(chosen),
  .width    = 240 * px,
});

constexpr auto price_tag = lv::label({.text = lv::bind(price), .style = lv::look<"price">});

constexpr auto drinks = lv::group(
  {.style = lv::look<"screen">}, title("Coffee"), menu, price_tag, go_to<step::cup>("Next")
);
//! [drinks]

}  // namespace

//! [page]
constinit const lv::page drinks_page = lv::page_of<drinks>;
//! [page]

}  // namespace coffee
