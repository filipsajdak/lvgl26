// Step 4 of the coffee machine tutorial (docs/research/tutorial-coffee.md): brewing takes time.
// A task fills a ring and counts the seconds down, and goes to the last page when it is done.
//
//   coffee_4    writes coffee_4.png: the brewing page, ten seconds in

#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/shown.hpp>

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
// What the reader chooses, and where the machine is.
enum class drink { espresso, lungo, cappuccino, latte };
enum class step { drinks, cup, brewing, enjoy };

struct strength {
  int value;
};

constinit lv::state chosen{drink::espresso};
constinit lv::state bold{strength{3}};
constinit lv::state cup{40 * ml};
constinit lv::state<step> current{step::drinks};
//! [state]

//! [parts]
// A page's title, and a button that goes to another step.
constexpr auto title(const char* text) {
  return lv::label({.text = text, .style = lv::look<"title">});
}

template <step To>
constexpr auto go_to(const char* text) {
  return lv::button(
    {.style = lv::look<"action">, .on_clicked = [] { current = To; }}, lv::label({.text = text})
  );
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

constexpr auto drinks = lv::group(
  {.style = lv::look<"screen">}, title("Coffee"), menu, price_tag, go_to<step::cup>("Next")
);
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

constexpr auto your_cup = lv::group(
  {.style = lv::look<"screen">}, title("Your cup"), strength_slider, size_slider, size_text,
  beans_text, go_to<step::brewing>("Brew")
);
//! [cup]

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
    .style = lv::look<"ring">,
  },
  lv::label({.align = "center", .text = lv::bind(left)})  // shows "25 s"
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

//! [enjoy]
constexpr auto ready = lv::label({.text = lv::bind(price)});

constexpr auto enjoy =
  lv::group({.style = lv::look<"screen">}, title("Enjoy"), ready, go_to<step::drinks>("Again"));
//! [enjoy]

//! [pages]
// A page slides in from the right, and the old one slides out to the left.
constexpr auto slide_in  = lv::transition({.x = {480 * px, 0 * px}}, 250 * lv::ms, lv::ease::out);
constexpr auto slide_out = lv::transition({.x = {0 * px, -480 * px}}, 250 * lv::ms, lv::ease::in);

constexpr auto ui = lv::group(
  lv::shown(
    current, {.enter = slide_in, .leave = slide_out}, lv::is<step::drinks>(drinks),
    lv::is<step::cup>(your_cup), lv::is<step::brewing>(brewing, {.on_shown = &start_brewing}),
    lv::is<step::enjoy>(enjoy)
  )
);
//! [pages]

constinit lv::frame<480, 320> frame;

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;

  // Ten and a half seconds of brewing, on a clock that steps 10 ms at a time: ten seconds have
  // been counted down.
  coffee::current = coffee::step::brewing;
  for (int i = 0; i < 1050; ++i) app.advance(10 * lv::ms), app.step();

  if (coffee::left.get() != 15 * mp_units::si::unit_symbols::s) return 2;
  return lv::write_png(display.screen(), "coffee_4.png") ? 0 : 3;
}
