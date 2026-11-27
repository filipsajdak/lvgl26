// Step 3 of the coffee machine tutorial (docs/tutorial.md): pages. The screen of the drinks and
// the screen of the cup are pages of one state; NEXT and BACK change it, and the new page slides in.
//
//   coffee_3    writes coffee_3.png: the page of the cup half way in

#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/shown.hpp>

#include <mp-units/systems/si.h>

#include <algorithm>
#include <array>
#include <cstddef>
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

//! [fonts]
// The machine's own typeface, drawn from its files by the compiler, and the fonts of the
// typography tokens as styles to merge.
constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char semibold_ttf[] = {
#embed "fonts/Barlow-SemiBold.ttf"
};
constexpr unsigned char bold_ttf[] = {
#embed "fonts/Barlow-Bold.ttf"
};
constexpr unsigned char condensed_bold_ttf[] = {
#embed "fonts/BarlowCondensed-Bold.ttf"
};
constexpr auto type = lv::typeface({
  {"Barlow", 500, barlow_ttf},
  {"Barlow", 600, semibold_ttf},
  {"Barlow", 700, bold_ttf},
  {"Barlow Condensed", 700, condensed_bold_ttf},
});

constexpr auto title_text    = lv::style({.text_font = type(tok.text.title)}) | tok.text.title;
constexpr auto label_text    = lv::style({.text_font = type(tok.text.label)}) | tok.text.label;
constexpr auto price_text    = lv::style({.text_font = type(tok.text.price)});
constexpr auto caption_text  = lv::style({.text_font = type(tok.text.caption)});
constexpr auto currency_text = lv::style({.text_font = type(tok.text.currency)});
constexpr auto note_text     = lv::style({.text_font = type(tok.text.note)});
constexpr auto sign_text     = lv::style({.text_font = type(tok.text.sign)});
constexpr auto numeral_text  = lv::style({.text_font = type(tok.text.numeral)});
constexpr auto body_text     = lv::style({.text_font = type(tok.text.body)});
constexpr auto overline_text =
  lv::style({.text_font = type(tok.text.overline)}) | tok.text.overline;
//! [fonts]

//! [state]
// What the reader chooses.
enum class drink { espresso, lungo, cappuccino, latte };

constinit lv::state chosen{drink::espresso};

// For each drink, whether it is the chosen one: a value made from the state.
template <drink D>
constinit lv::computed picked{[](drink d) { return d == D; }, chosen};
//! [state]

//! [values]
// The strength of a cup, from 1 to 5: a struct of one member says what the number is.
struct strength {
  int value;
};

constexpr strength mildest{1};
constexpr strength strongest{5};

// The size, a quantity in millilitres, within what the drink can be made in.
constexpr auto ml = mp_units::si::milli<mp_units::si::litre>;
using millilitres = decltype(40 * ml);

struct cup_sizes {
  millilitres least, usual, most;
};
constexpr cup_sizes sizes[] = {
  {20 * ml, 40 * ml, 60 * ml},
  {80 * ml, 110 * ml, 150 * ml},
  {140 * ml, 180 * ml, 240 * ml},
  {180 * ml, 240 * ml, 300 * ml},
};
constexpr auto size_step = 10 * ml;

constexpr cup_sizes sizes_of(drink d) { return sizes[std::to_underlying(d)]; }

constinit lv::state bold{strength{3}};
constinit lv::state cup{sizes_of(drink::espresso).usual};

// The beans, a quantity in grams, made from the strength.
constexpr auto least_dose = 6 * g;  // the beans of the mildest cup
constexpr auto dose_step  = 2 * g;  // what one step of strength adds

constinit lv::computed beans{[](strength s) { return least_dose + s.value * dose_step; }, bold};

// The steppers: what each button does, and when it can go no further.
void milder() { bold = strength{std::max(mildest.value, bold.get().value - 1)}; }
void stronger() { bold = strength{std::min(strongest.value, bold.get().value + 1)}; }
void smaller() { cup = std::max(sizes_of(chosen.get()).least, cup.get() - size_step); }
void larger() { cup = std::min(sizes_of(chosen.get()).most, cup.get() + size_step); }

constinit lv::computed at_mildest{[](strength s) { return s.value <= mildest.value; }, bold};
constinit lv::computed at_strongest{[](strength s) { return s.value >= strongest.value; }, bold};
constinit lv::computed at_smallest{
  [](drink d, millilitres c) { return c <= sizes_of(d).least; }, chosen, cup
};
constinit lv::computed at_largest{
  [](drink d, millilitres c) { return c >= sizes_of(d).most; }, chosen, cup
};
//! [values]

//! [screens]
// Where the machine is: one page for each screen, and a button that goes to one.
enum class screen { drinks, your_cup };

constinit lv::state<screen> current{screen::drinks};

template <screen To>
void go() {
  current = To;
}
//! [screens]

//! [photos]
// A photo of each drink, cut out, at 1.5 x target: the compiler decodes it.
constexpr unsigned char espresso_png[] = {
#embed "../assets/espresso-72.png"
};
constexpr unsigned char lungo_png[] = {
#embed "../assets/lungo-72.png"
};
constexpr unsigned char cappuccino_png[] = {
#embed "../assets/cappuccino-72.png"
};
constexpr unsigned char latte_png[] = {
#embed "../assets/latte-72.png"
};

// What a tile shows of each drink, in the order of the enum.
constexpr decltype(lv::image_file<espresso_png>) photos[] = {
  lv::image_file<espresso_png>,
  lv::image_file<lungo_png>,
  lv::image_file<cappuccino_png>,
  lv::image_file<latte_png>,
};
constexpr const char* names[]  = {"ESPRESSO", "LUNGO", "CAPPUCCINO", "LATTE"};
constexpr const char* prices[] = {"2.20", "2.40", "3.10", "3.40"};

// The chosen drink's photo, name and price, for the summary.
constinit lv::computed photo{[](drink d) { return photos[std::to_underlying(d)]; }, chosen};
constinit lv::computed name{[](drink d) { return names[std::to_underlying(d)]; }, chosen};
constinit lv::computed price{[](drink d) { return prices[std::to_underlying(d)]; }, chosen};

// What the fields say of the values.
constexpr std::array strength_words{
  "1 of 5, Mild", "2 of 5, Light", "3 of 5, Medium", "4 of 5, Strong", "5 of 5, Intense"
};
constexpr const char* size_ranges[] = {
  "20 to 60 mL", "80 to 150 mL", "140 to 240 mL", "180 to 300 mL"
};

constinit lv::computed strength_word{
  [](strength s) { return strength_words[static_cast<std::size_t>(s.value - mildest.value)]; }, bold
};
constinit lv::computed size_range{
  [](drink d) { return size_ranges[std::to_underlying(d)]; }, chosen
};
//! [photos]

//! [parts]
// The header: the title top left, and the step top right as dots and "n / 4".
constexpr auto dot(bool on) {
  return lv::group({.style = on ? lv::look<"dot dot-on"> : lv::look<"dot">});
}

constexpr auto header(const char* title, int step, const char* count) {
  return lv::group(
    {.id = "header", .style = lv::look<"header">},
    lv::label({.id = "title", .text = title, .style = lv::look<"title"> | title_text}),
    lv::group(
      {.size = {lv::size_content, lv::size_content}, .style = lv::look<"dots">},
      dot(step >= 1),
      dot(step >= 2),
      dot(step >= 3),
      dot(step >= 4),
      lv::label({.text = count, .style = lv::look<"step"> | caption_text})
    )
  );
}

// The footer: going back bottom left, going on bottom right.
constexpr auto back(const char* text, lv::tree_handler auto go_to) {
  return lv::button(
    {.style = lv::look<"ghost">, .on_clicked = go_to},
    lv::label({.text = text, .style = label_text})
  );
}

constexpr auto primary(const char* text, lv::tree_handler auto go_to) {
  return lv::button(
    {.style = lv::look<"primary">, .on_clicked = go_to},
    lv::label({.text = text, .style = label_text})
  );
}

constexpr auto primary(const char* text) {
  return lv::button({.style = lv::look<"primary">}, lv::label({.text = text, .style = label_text}));
}

// A screen with nothing to go back to keeps the place empty.
constexpr auto no_back = lv::group({.size = {lv::size_content, lv::size_content}});

constexpr auto footer(lv::tree_node auto back, lv::tree_node auto next) {
  return lv::group({.id = "footer", .style = lv::look<"footer">}, back, next);
}
//! [parts]

//! [drinks]
// A tile: the photo, the name and the price. It is checked while its drink is the chosen one.
template <drink D>
constexpr auto tile() {
  return lv::button(
    {
      .checked    = lv::bind(picked<D>),
      .style      = lv::look<"tile">,
      .on_clicked = [] { chosen = D, cup = sizes_of(D).usual; },
    },
    lv::image({.src = photos[std::to_underlying(D)]}),
    lv::group(
      {.size = {lv::size_content, lv::size_content}, .style = lv::look<"tile-text">},
      lv::label({.text = names[std::to_underlying(D)], .style = label_text}),
      lv::group(
        {.size = {lv::size_content, lv::size_content}, .style = lv::look<"price-row">},
        lv::label({.text = prices[std::to_underlying(D)], .style = price_text}),
        lv::label({.text = "EUR", .style = lv::look<"currency"> | currency_text})
      )
    )
  );
}

constexpr auto tiles = lv::group(
  {.style = lv::look<"tiles">},
  lv::group({.style = lv::look<"tile-row">}, tile<drink::espresso>(), tile<drink::lungo>()),
  lv::group({.style = lv::look<"tile-row">}, tile<drink::cappuccino>(), tile<drink::latte>())
);

constexpr auto drinks = lv::group(
  {.style = lv::look<"page">}, header("CHOOSE A DRINK", 1, "1 / 4"), tiles,
  footer(no_back, primary("NEXT", &go<screen::your_cup>))
);
//! [drinks]

//! [cup]
// A stepper: -, what is set, +. A button that can go no further is disabled.
constexpr auto stepper(
  const lv::dependency auto& at_least, const lv::dependency auto& at_most,
  lv::tree_handler auto less, lv::tree_handler auto more, lv::tree_node auto shown
) {
  return lv::group(
    {.style = lv::look<"stepper">},
    lv::button(
      {.disabled = lv::bind(at_least), .style = lv::look<"square">, .on_clicked = less},
      lv::label({.text = "-", .style = sign_text})
    ),
    shown,
    lv::button(
      {.disabled = lv::bind(at_most), .style = lv::look<"square">, .on_clicked = more},
      lv::label({.text = "+", .style = sign_text})
    )
  );
}

// A field: its name and what it says of its value, over its control.
constexpr auto field(
  const char* text, const lv::dependency auto& note, lv::tree_node auto control
) {
  return lv::group(
    {.size = {100 * pct, lv::size_content}, .style = lv::look<"field">},
    lv::group(
      {.size = {100 * pct, lv::size_content}, .style = lv::look<"field-top">},
      lv::label({.text = text, .style = lv::look<"overline"> | overline_text}),
      lv::label({.text = lv::bind(note), .style = note_text})
    ),
    control
  );
}

// The strength as five segments: a segment is checked up to the strength.
template <int I>
constinit lv::computed segment_on{[](strength s) { return s.value >= I; }, bold};

template <int I>
constexpr auto segment() {
  return lv::group({.checked = lv::bind(segment_on<I>), .style = lv::look<"segment">});
}

constexpr auto segments = lv::group(
  {.style = lv::look<"segments">}, segment<1>(), segment<2>(), segment<3>(), segment<4>(),
  segment<5>()
);

constexpr auto size_value = lv::group(
  {.style = lv::look<"value">}, lv::label({.text = lv::bind(cup, "{}"), .style = numeral_text}),
  lv::label({.text = "mL", .style = lv::look<"unit"> | body_text})
);

constexpr auto fields = lv::group(
  {.style = lv::look<"fields">},
  field("STRENGTH", strength_word, stepper(at_mildest, at_strongest, &milder, &stronger, segments)),
  field("SIZE", size_range, stepper(at_smallest, at_largest, &smaller, &larger, size_value))
);

// The summary of the cup: the chosen drink's photo, name, beans and price.
constexpr auto summary = lv::group(
  {.id = "summary", .style = lv::look<"summary">},
  lv::group(
    {.size = {100 * pct, lv::size_content}, .style = lv::look<"photo-box">},
    lv::image({.id = "summary-photo", .src = lv::bind(photo)})
  ),
  lv::label({.text = lv::bind(name), .style = lv::look<"summary-name"> | label_text}),
  lv::label({.text = lv::bind(beans, "{} g of beans"), .style = lv::look<"beans"> | caption_text}),
  lv::group({.style = lv::look<"spacer">}),
  lv::group(
    {.size = {lv::size_content, lv::size_content}, .style = lv::look<"price-row">},
    lv::label({.text = lv::bind(price), .style = numeral_text}),
    lv::label({.text = "EUR", .style = lv::look<"unit"> | body_text})
  )
);

constexpr auto your_cup = lv::group(
  {.style = lv::look<"page">}, header("YOUR CUP", 2, "2 / 4"),
  lv::group({.style = lv::look<"cup">}, fields, summary),
  footer(back("BACK", &go<screen::drinks>), primary("BREW"))
);
//! [cup]

//! [pages]
// The page of the current screen. A new page slides in from the right, and the old one slides out
// to the left, in the time and the curve of the token file.
constexpr auto width = tok.size.screen_width->raw();
constexpr auto slide_in =
  lv::transition({.x = {width * px, 0 * px}}, *tok.duration.enter, *tok.ease.out);
constexpr auto slide_out =
  lv::transition({.x = {0 * px, -width * px}}, *tok.duration.enter, *tok.ease.out);

constexpr auto ui = lv::group(
  lv::shown(
    current, {.enter = slide_in, .leave = slide_out}, lv::is<screen::drinks>(drinks),
    lv::is<screen::your_cup>(your_cup)
  )
);
//! [pages]

// The display's memory: the screen of the token file, size.screen-width x size.screen-height,
// which main checks.
constinit lv::frame<480, 320> frame;

// Moves the clock on by 10 ms and draws, `count` times.
void steps(lv::application& app, int count) {
  for (int i = 0; i < count; ++i) app.advance(10 * lv::ms), app.step();
}

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;
  coffee::steps(app, 5);  // past the first refresh
  const auto screen = display.screen();
  if (screen.get<"width">() != coffee::width) return 8;
  if (screen.get<"height">() != coffee::tok.size.screen_height->raw()) return 8;

  // The reader goes on to the cup: the page slides in, on a clock that steps 10 ms at a time.
  // Half way, the picture shows both pages.
  coffee::go<coffee::screen::your_cup>();
  coffee::steps(app, 10);
  if (!lv::write_png(display.screen(), "coffee_3.png")) return 3;

  // When the slide has ended, BACK goes back the same way.
  coffee::steps(app, 30);
  coffee::go<coffee::screen::drinks>();
  coffee::steps(app, 30);
  return coffee::current.get() == coffee::screen::drinks ? 0 : 2;
}
