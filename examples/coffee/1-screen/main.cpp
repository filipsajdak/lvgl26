// Step 1 of the coffee machine tutorial (docs/tutorial.md): one screen, as one
// tree. The drinks are a state of an enum, and the price follows it.
//
//   coffee_1    writes coffee_1.png

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

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

//! [state]
// What the reader chooses.
enum class drink { espresso, lungo, cappuccino, latte };

constinit lv::state chosen{drink::espresso};
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

constexpr auto ui = lv::group({.style = lv::look<"screen">}, title("Coffee"), menu, price_tag);
//! [drinks]

constinit lv::frame<480, 320> frame;

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;

  // The reader picks a lungo: the price follows.
  coffee::chosen = coffee::drink::lungo;
  app.step();

  if (coffee::price.get() != coffee::prices[1]) return 2;
  return lv::write_png(display.screen(), "coffee_1.png") ? 0 : 3;
}
