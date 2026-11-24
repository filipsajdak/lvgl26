// Step 2 of the coffee machine tutorial (docs/research/tutorial-coffee.md): values that say what
// they are. Strength is a struct of one member, the size a quantity in millilitres, and the beans
// are made from them.
//
//   coffee_2    writes coffee_2.png

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

#include <mp-units/systems/si.h>

#include <utility>

namespace coffee {

constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

}  // namespace coffee

// The design of the program: the looks read this token file.
template <>
inline constexpr lv::design_of<decltype(coffee::tok)> lv::design<lv::override>{
  .tokens = coffee::tok
};

namespace coffee {

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

constexpr auto ml = mp_units::si::milli<mp_units::si::litre>;

//! [state]
// What the reader chooses.
enum class drink { espresso, lungo, cappuccino, latte };

struct strength {
  int value;
};

constinit lv::state chosen{drink::espresso};
constinit lv::state bold{strength{3}};
constinit lv::state cup{40 * ml};
//! [state]

//! [parts]
// A page's title.
constexpr auto title(const char* text) {
  return lv::label({.text = text, .style = lv::look<"title">});
}
//! [parts]

//! [drinks]
// The price of each drink, in the order of the enum.
constexpr const char* prices[] = {"2.20 EUR", "2.40 EUR", "3.10 EUR", "3.30 EUR"};

constinit lv::computed price{[](drink d) { return prices[std::to_underlying(d)]; }, chosen};

constexpr auto menu = lv::dropdown({
  .options  = lv::options_of<drink>,
  .selected = lv::bind(chosen),
  .width    = 240 * px,
});

constexpr auto price_tag = lv::label({.text = lv::bind(price), .style = lv::look<"price">});
//! [drinks]

//! [cup]
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

constexpr auto ui = lv::group(
  {.style = lv::look<"screen">}, title("Coffee"), menu, price_tag, strength_slider, size_slider,
  size_text, beans_text
);
//! [cup]

constinit lv::frame<480, 320> frame;

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;

  // A stronger and larger cup: the beans follow the strength.
  coffee::bold = coffee::strength{4};
  coffee::cup  = 60 * coffee::ml;
  app.step();

  if (coffee::beans.get() != 14 * mp_units::si::unit_symbols::g) return 2;
  return lv::write_png(display.screen(), "coffee_2.png") ? 0 : 3;
}
