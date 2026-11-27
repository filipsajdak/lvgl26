// Step 4 of the coffee machine tutorial (docs/research/tutorial-coffee-draft.md): brewing takes
// time. While the brewing page is shown, a task fills the ring and counts the seconds down; holding
// the ring for a second cancels, and so does CANCEL. The finished cup settles in, steam rises over
// it, and the machine goes back to the drinks on its own.
//
//   coffee_4    checks the brewing, the hold and the screens, then writes coffee_4.png (the
//               brewing, 20 seconds before the end) and coffee_4_enjoy.png (the finished cup)

#include <lv/font.hpp>
#include <lv/gesture.hpp>
#include <lv/image.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/pointer.hpp>
#include <lv/shown.hpp>

#include <mp-units/systems/si.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
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
// The machine's own typeface, drawn from its files by the compiler. Each font is drawn once, and
// the styles that need it name it: a note is set in the overline's face, without its tracking. The
// condensed face sets figures and names in capitals only, so it holds those characters.
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
  {"Barlow", 500, barlow_ttf, lv::ascii_plus<"€·">},
  {"Barlow", 600, semibold_ttf, lv::ascii_plus<"€">},
  {"Barlow", 700, bold_ttf, lv::ascii_plus<"·">},
  {"Barlow Condensed", 700, condensed_bold_ttf, "0123456789. ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
});

constexpr auto caption_font  = type(tok.text.caption);
constexpr auto body_font     = type(tok.text.body);
constexpr auto title_font    = type(tok.text.title);
constexpr auto currency_font = type(tok.text.currency);
constexpr auto label_font    = type(tok.text.label);
constexpr auto overline_font = type(tok.text.overline);
constexpr auto sign_font     = type(tok.text.sign);
constexpr auto price_font    = type(tok.text.price);
constexpr auto numeral_font  = type(tok.text.numeral);
constexpr auto seconds_font  = type(tok.text.numeral_large);

constexpr auto title_text    = lv::style({.text_font = title_font}) | tok.text.title;
constexpr auto label_text    = lv::style({.text_font = label_font}) | tok.text.label;
constexpr auto fact_text     = lv::style({.text_font = label_font});
constexpr auto price_text    = lv::style({.text_font = price_font});
constexpr auto caption_text  = lv::style({.text_font = caption_font});
constexpr auto currency_text = lv::style({.text_font = currency_font});
constexpr auto note_text     = lv::style({.text_font = overline_font}) | tok.text.note;
constexpr auto sign_text     = lv::style({.text_font = sign_font});
constexpr auto numeral_text  = lv::style({.text_font = numeral_font});
constexpr auto body_text     = lv::style({.text_font = body_font});
constexpr auto overline_text = lv::style({.text_font = overline_font}) | tok.text.overline;
constexpr auto headline_text = lv::style({.text_font = numeral_font}) | tok.text.headline;
constexpr auto seconds_text  = lv::style({.text_font = seconds_font}) | tok.text.numeral_large;
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
enum class screen { drinks, your_cup, brewing, enjoy };

constinit lv::state<screen> current{screen::drinks};

template <screen To>
void go() {
  current = To;
}
//! [screens]

//! [photos]
// A photo of each drink, cut out, at 1.5 x target for a tile and at 3 x target for the finished
// cup: the compiler decodes them.
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
constexpr unsigned char espresso_hero_png[] = {
#embed "../assets/espresso-144.png"
};
constexpr unsigned char lungo_hero_png[] = {
#embed "../assets/lungo-144.png"
};
constexpr unsigned char cappuccino_hero_png[] = {
#embed "../assets/cappuccino-144.png"
};
constexpr unsigned char latte_hero_png[] = {
#embed "../assets/latte-144.png"
};

// What the screens show of each drink, in the order of the enum.
constexpr decltype(lv::image_file<espresso_png>) photos[] = {
  lv::image_file<espresso_png>,
  lv::image_file<lungo_png>,
  lv::image_file<cappuccino_png>,
  lv::image_file<latte_png>,
};
constexpr decltype(lv::image_file<espresso_hero_png>) heroes[] = {
  lv::image_file<espresso_hero_png>,
  lv::image_file<lungo_hero_png>,
  lv::image_file<cappuccino_hero_png>,
  lv::image_file<latte_hero_png>,
};
constexpr const char* names[]       = {"ESPRESSO", "LUNGO", "CAPPUCCINO", "LATTE"};
constexpr const char* drink_names[] = {"Espresso", "Lungo", "Cappuccino", "Latte"};
constexpr const char* prices[]      = {"2.20", "2.40", "3.10", "3.40"};

// The chosen drink's photos, name and price.
constinit lv::computed photo{[](drink d) { return photos[std::to_underlying(d)]; }, chosen};
constinit lv::computed hero{[](drink d) { return heroes[std::to_underlying(d)]; }, chosen};
constinit lv::computed name{[](drink d) { return names[std::to_underlying(d)]; }, chosen};
constinit lv::computed drink_name{
  [](drink d) { return drink_names[std::to_underlying(d)]; }, chosen
};
constinit lv::computed price{[](drink d) { return prices[std::to_underlying(d)]; }, chosen};

// What the fields say of the values.
constexpr std::array strength_words{
  "1 of 5 · Mild", "2 of 5 · Light", "3 of 5 · Medium", "4 of 5 · Strong", "5 of 5 · Intense"
};
constexpr std::array strength_names{"Mild", "Light", "Medium", "Strong", "Intense"};
constexpr const char* size_ranges[] = {
  "20 to 60 mL", "80 to 150 mL", "140 to 240 mL", "180 to 300 mL"
};

constexpr std::size_t step_of(strength s) {
  return static_cast<std::size_t>(s.value - mildest.value);
}

constinit lv::computed strength_word{[](strength s) { return strength_words[step_of(s)]; }, bold};
constinit lv::computed strength_name{[](strength s) { return strength_names[step_of(s)]; }, bold};
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

// A button with an outline: what stops something, where going back would be.
constexpr auto outline(const char* text, lv::tree_handler auto go_to) {
  return lv::button(
    {.style = lv::look<"outline">, .on_clicked = go_to},
    lv::label({.text = text, .style = label_text})
  );
}

// A place in the footer with nothing in it.
constexpr auto nothing = lv::group({.size = {lv::size_content, lv::size_content}});

constexpr auto footer(lv::tree_node auto left, lv::tree_node auto right) {
  return lv::group({.id = "footer", .style = lv::look<"footer">}, left, right);
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
        lv::label({.text = "€", .style = lv::look<"currency"> | currency_text})
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
  footer(nothing, primary("NEXT", &go<screen::your_cup>))
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
    lv::label({.text = "€", .style = lv::look<"unit"> | body_text})
  )
);

constexpr auto your_cup = lv::group(
  {.style = lv::look<"page">}, header("YOUR CUP", 2, "2 / 4"),
  lv::group({.style = lv::look<"cup">}, fields, summary),
  footer(back("BACK", &go<screen::drinks>), primary("BREW", &go<screen::brewing>))
);
//! [cup]

//! [brewing]
// How long each drink brews, in the order of the enum: a value of the program, in seconds.
using seconds                  = decltype(1 * s);
constexpr seconds brew_times[] = {25 * s, 35 * s, 45 * s, 50 * s};

constexpr seconds brew_time(drink d) { return brew_times[std::to_underlying(d)]; }

constinit lv::state<seconds> left{brew_time(drink::espresso)};

// What the machine is doing, by how much of the time has gone: grinding for the first fifth,
// extracting up to nine tenths, then finishing.
constexpr const char* stage_at(seconds gone, seconds total) {
  if (gone * 5 < total) return "GRINDING";
  if (gone * 10 < total * 9) return "EXTRACTING";
  return "FINISHING";
}

constinit lv::computed stage{
  [](drink d, seconds still) { return stage_at(brew_time(d) - still, brew_time(d)); }, chosen, left
};

// The ring counts in hundredths of the brewing.
constexpr int ring_full = 100;

constexpr int filled(seconds gone, seconds total) {
  return gone.numerical_value_in(s) * ring_full / total.numerical_value_in(s);
}

// Each second moves the ring on for a second and counts one down; at the end the cup is ready.
lv::task<> count_down(lv::view<"arc"> ring) {
  const seconds total = brew_time(chosen.get());
  for (left = total; left.get() > 0 * s; left -= 1 * s) {
    const seconds gone = total - left.get();
    co_await lv::animate(
      ring, {.value = {filled(gone, total), filled(gone + 1 * s, total)}}, 1 * sec
    );
  }
  go<screen::enjoy>();
}

// A finger that stays on the ring for a second cancels; one that leaves sooner does not.
lv::task<> hold_to_cancel(lv::view<"obj"> ring) {
  while (!co_await lv::hold(ring, 1 * sec)) {
  }
  go<screen::your_cup>();
}

constexpr auto ring = lv::group(
  {.id = "ring-box", .style = lv::look<"ring-box">},
  lv::arc({
    .id        = "ring",
    .bg_angles = {0 * lv::deg, 360 * lv::deg},
    .clickable = false,
    .range     = {0, ring_full},
    .rotation  = 270 * lv::deg,
    .value     = 0,
    .style     = lv::look<"ring"> | lv::style({.arc_rounded = false}) |
             lv::on("indicator", {.arc_rounded = false}),
  }),
  // The middle passes a press on to the ring's box, which the hold waits on.
  lv::group(
    {.clickable = false, .style = lv::look<"ring-centre">},
    lv::label({.text = lv::bind(left, "{}"), .style = seconds_text}),
    lv::label({.text = "SECONDS LEFT", .style = lv::look<"seconds-label"> | overline_text})
  )
);

// A fact of the cup: its name, and its value.
constexpr auto fact(const char* what, const lv::dependency auto& value) {
  return lv::group(
    {.size = {100 * pct, lv::size_content}, .style = lv::look<"fact">},
    lv::label({.text = what, .style = lv::look<"fact-name"> | body_text}),
    lv::label({.text = lv::bind(value), .style = fact_text})
  );
}

constexpr auto side = lv::group(
  {.size = {100 * pct, lv::size_content}, .style = lv::look<"side">},
  lv::group(
    {.size = {100 * pct, lv::size_content}, .style = lv::look<"stage">},
    lv::label({.text = "NOW", .style = lv::look<"overline"> | overline_text}),
    lv::label({.id = "stage", .text = lv::bind(stage), .style = label_text})
  ),
  lv::group(
    {.size = {100 * pct, lv::size_content}, .style = lv::look<"facts">}, fact("Drink", drink_name),
    fact("Strength", strength_name), fact("Size", cup)
  )
);

constexpr auto brewing = lv::group(
  {.style = lv::look<"page">}, header("BREWING", 3, "3 / 4"),
  lv::group({.style = lv::look<"brew">}, ring, side),
  footer(outline("CANCEL", &go<screen::your_cup>), nothing)
);

// The tasks of the brewing belong to the ring: leaving the page ends them.
void start_brewing(lv::held<brewing> page) {
  lv::spawn(count_down(page->get<"ring">()), page->get<"ring">());
  lv::spawn(hold_to_cancel(page->get<"ring-box">()), page->get<"ring-box">());
}
//! [brewing]

//! [enjoy]
// A wisp of steam: one small picture, used three times and tinted by its look.
constexpr unsigned char steam_png[] = {
#embed "../assets/steam-96.png"
};
constexpr auto steam = lv::mask_file<steam_png>;

// A latte has no steam over it.
constinit lv::computed no_steam{[](drink d) { return d == drink::latte; }, chosen};

// The movements of the mock-up's keyframes, in percent: the cup lands from 60 percent of its size;
// a wisp grows from 90 x 60 to 150 x 110 percent of its own, is thickest at 30 percent of its loop,
// and the three start 0, 40 and 70 percent of a loop apart. LVGL counts a scale in 256ths of the
// size.
constexpr int per_cent      = 100;
constexpr int scale_whole   = 256;
constexpr int landing_scale = 60;
constexpr int wisp_from_x = 90, wisp_to_x = 150, wisp_from_y = 60, wisp_to_y = 110;
constexpr int wisp_peak = 30;
constexpr std::array wisp_starts{0, 40, 70};

constexpr int scale_of(int share) { return scale_whole * share / per_cent; }

constexpr lv::duration part_of(lv::duration whole, int share) {
  return {whole.ms * static_cast<std::uint32_t>(share) / per_cent};
}

// A wisp at its thickest: the opacity token as a percentage.
constexpr auto wisp_opa = static_cast<int>(*tok.opacity.steam * per_cent) * pct;

// The cup lands: it fades in, grows and drops into place.
lv::task<> fade_in(lv::view<"image"> cup_photo) {
  co_await lv::animate(
    cup_photo, {.opa = {0 * pct, 100 * pct}}, *tok.duration.settle, *tok.ease.back
  );
}

lv::task<> settle(lv::view<"image"> cup_photo) {
  lv::spawn(fade_in(cup_photo), cup_photo);
  lv::spawn(
    [](lv::view<"image"> landing) -> lv::task<> {
      co_await lv::animate(
        landing,
        {.scale = {scale_of(landing_scale), scale_whole}},
        *tok.duration.settle,
        *tok.ease.back
      );
    }(cup_photo),
    cup_photo
  );
  co_await lv::animate(
    cup_photo,
    {.translate_y = {tok.space._4->raw() * px, 0 * px}},
    *tok.duration.settle,
    *tok.ease.back
  );
}

// A wisp rises and grows over one loop, and fades in and out on the way.
lv::task<> drift(lv::view<"image"> wisp) {
  const auto loop = *tok.duration.steam;
  lv::spawn(
    [](lv::view<"image"> w, lv::duration span) -> lv::task<> {
      co_await lv::animate(
        w, {.scale_x = {scale_of(wisp_from_x), scale_of(wisp_to_x)}}, span, *tok.ease.in_out
      );
    }(wisp, loop),
    wisp
  );
  lv::spawn(
    [](lv::view<"image"> w, lv::duration span) -> lv::task<> {
      co_await lv::animate(
        w, {.scale_y = {scale_of(wisp_from_y), scale_of(wisp_to_y)}}, span, *tok.ease.in_out
      );
    }(wisp, loop),
    wisp
  );
  co_await lv::animate(
    wisp,
    {.translate_y = {tok.space._3->raw() * px, -tok.space._4->raw() * px}},
    loop,
    *tok.ease.in_out
  );
}

lv::task<> rise(lv::view<"image"> wisp, std::size_t nth) {
  const auto loop = *tok.duration.steam;
  co_await lv::sleep(part_of(loop, wisp_starts[nth]));
  for (;;) {
    lv::spawn(drift(wisp), wisp);
    co_await lv::animate(
      wisp, {.opa = {0 * pct, wisp_opa}}, part_of(loop, wisp_peak), *tok.ease.in_out
    );
    co_await lv::animate(
      wisp, {.opa = {wisp_opa, 0 * pct}}, part_of(loop, per_cent - wisp_peak), *tok.ease.in_out
    );
  }
}

// The name slides up into place after the cup has landed.
lv::task<> slide_up(lv::view<"label"> label) {
  co_await lv::sleep(*tok.duration.settle);
  co_await lv::animate(
    label, {.translate_y = {tok.space._3->raw() * px, 0 * px}}, *tok.duration.enter, *tok.ease.out
  );
}

// The machine goes back to the drinks on its own, as the page says.
constexpr auto back_after = 10 * sec;

lv::task<> wait_and_go_back() {
  co_await lv::sleep(back_after);
  go<screen::drinks>();
}

constexpr auto finished_cup = lv::group(
  {.style = lv::look<"hero">}, lv::image({.id = "hero", .src = lv::bind(hero)}),
  lv::image({
    .id     = "steam-1",
    .hidden = lv::bind(no_steam),
    .pivot  = {50 * pct, 100 * pct},
    .src    = steam,
    .style  = lv::look<"steam steam-1">,
  }),
  lv::image({
    .id     = "steam-2",
    .hidden = lv::bind(no_steam),
    .pivot  = {50 * pct, 100 * pct},
    .src    = steam,
    .style  = lv::look<"steam steam-2">,
  }),
  lv::image({
    .id     = "steam-3",
    .hidden = lv::bind(no_steam),
    .pivot  = {50 * pct, 100 * pct},
    .src    = steam,
    .style  = lv::look<"steam steam-3">,
  })
);

constexpr auto ready = lv::group(
  {.size = {lv::size_content, lv::size_content}, .style = lv::look<"done-text">},
  lv::label({.text = "READY", .style = lv::look<"ready"> | overline_text}),
  lv::label(
    {.id = "headline", .text = lv::bind(name), .style = lv::look<"headline"> | headline_text}
  ),
  lv::group(
    {.size = {lv::size_content, lv::size_content}, .style = lv::look<"line">},
    lv::label({.text = lv::bind(cup), .style = caption_text}),
    lv::label({.text = " · ", .style = caption_text}),
    lv::label({.text = lv::bind(strength_name), .style = caption_text}),
    lv::label({.text = " · ", .style = caption_text}),
    lv::label({.text = lv::bind(price), .style = caption_text}),
    lv::label({.text = " €", .style = caption_text})
  ),
  lv::label({.text = "Back to the drinks in 10 s", .style = lv::look<"line"> | caption_text})
);

constexpr auto enjoy = lv::group(
  {.style = lv::look<"page">}, header("ENJOY", 4, "4 / 4"),
  lv::group({.style = lv::look<"done">}, finished_cup, ready),
  footer(nothing, primary("NEW DRINK", &go<screen::drinks>))
);

// What moves on the finished cup belongs to the page's widgets: leaving the page ends it.
void serve(lv::held<enjoy> page) {
  const auto cup_photo = page->get<"hero">();
  lv::spawn(settle(cup_photo), cup_photo);
  lv::spawn(rise(page->get<"steam-1">(), 0), page->get<"steam-1">());
  lv::spawn(rise(page->get<"steam-2">(), 1), page->get<"steam-2">());
  lv::spawn(rise(page->get<"steam-3">(), 2), page->get<"steam-3">());
  lv::spawn(slide_up(page->get<"headline">()), page->get<"headline">());
  lv::spawn(wait_and_go_back(), cup_photo);
}
//! [enjoy]

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
    lv::is<screen::your_cup>(your_cup),
    lv::is<screen::brewing>(brewing, {.on_shown = &start_brewing}),
    lv::is<screen::enjoy>(enjoy, {.on_shown = &serve})
  )
);
//! [pages]

// The display's memory: the screen of the token file, size.screen-width x size.screen-height.
constinit lv::frame<tok.size.screen_width->raw(), tok.size.screen_height->raw()> frame;

// Moves the clock on by 10 ms and draws, `count` times.
void steps(lv::application& app, int count) {
  for (int i = 0; i < count; ++i) app.advance(10 * lv::ms), app.step();
}

//! [checks]
// Where the tokens put the middle of the ring: the left margin plus half the ring, and the middle
// of the content between the header and the gap above the footer.
constexpr lv::point ring_middle{
  tok.layout.margin->raw() + tok.size.ring->raw() / 2,
  tok.layout.header_height->raw() +
    (tok.size.screen_height->raw() - tok.layout.header_height->raw() -
     tok.layout.footer_height->raw() - tok.layout.gap->raw()) /
      2,
};

// The mock-up's cup brews: a cappuccino, Medium, 180 mL, until 20 of its 45 seconds are left. The
// ring is then 55 percent full, and the machine is extracting.
int brew_like_the_mock_up(lv::application& app, lv::obj screen) {
  chosen = drink::cappuccino, cup = sizes_of(drink::cappuccino).usual;
  go<screen::brewing>();
  for (int i = 0; i < 3000 && left.get() != 20 * s; ++i) steps(app, 1);
  if (left.get() != 20 * s) return 2;
  if (std::string_view{stage.get()} != "EXTRACTING") return 2;
  return lv::write_png(screen, "coffee_4.png") ? 0 : 3;
}

// A finger lifted before the second does not cancel; one that stays a second does.
int hold_the_ring(lv::application& app, lv::pointer<>& finger) {
  finger.press(ring_middle);
  steps(app, 40);
  finger.release();
  steps(app, 10);
  if (current.get() != screen::brewing) return 9;
  finger.press(ring_middle);
  steps(app, 120);
  finger.release();
  steps(app, 10);
  return current.get() == screen::your_cup ? 0 : 10;
}

// Brewed to the end, the cup is ready; ten seconds later the machine shows the drinks again.
int brew_to_the_end(lv::application& app) {
  go<screen::brewing>();
  for (int i = 0; i < 5000 && current.get() != screen::enjoy; ++i) steps(app, 1);
  return current.get() == screen::enjoy ? 0 : 11;
}
//! [checks]

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;
  lv::pointer finger{display};
  coffee::steps(app, 5);  // past the first refresh
  if (display.screen().get<"width">() != coffee::width) return 8;

  if (const int wrong = coffee::brew_like_the_mock_up(app, display.screen())) return wrong;
  if (const int wrong = coffee::hold_the_ring(app, finger)) return wrong;
  if (const int wrong = coffee::brew_to_the_end(app)) return wrong;

  // The cup has landed and the name is in place: the picture of the finished cup.
  coffee::steps(app, 100);
  if (!lv::write_png(display.screen(), "coffee_4_enjoy.png")) return 3;
  coffee::steps(app, 1000);
  return coffee::current.get() == coffee::screen::drinks ? 0 : 12;
}
