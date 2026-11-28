// Step 1 of the coffee machine tutorial (docs/research/tutorial-coffee-draft.md): the screen where
// the reader chooses a drink, in the layout of the mock-up (design/coffee). The drinks are a state
// of an enum, and each tile follows it: the chosen one fills with the loud colour.
//
//   coffee_1    checks the layout against the token file, then writes coffee_1.png

#include <lv/font.hpp>
#include <lv/image.hpp>
#include <lv/look.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

#include <cstdlib>
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
  {"Barlow", 600, semibold_ttf, lv::ascii_plus<"€">},
  {"Barlow", 700, bold_ttf},
  {"Barlow Condensed", 700, condensed_bold_ttf},
});

constexpr auto title_text    = lv::style({.text_font = type(tok.text.title)}) | tok.text.title;
constexpr auto label_text    = lv::style({.text_font = type(tok.text.label)}) | tok.text.label;
constexpr auto price_text    = lv::style({.text_font = type(tok.text.price)});
constexpr auto caption_text  = lv::style({.text_font = type(tok.text.caption)});
constexpr auto currency_text = lv::style({.text_font = type(tok.text.currency)});
//! [fonts]

//! [state]
// What the reader chooses.
enum class drink { espresso, lungo, cappuccino, latte };

constinit lv::state chosen{drink::espresso};

// For each drink, whether it is the chosen one: a value made from the state.
template <drink D>
constinit lv::computed picked{[](drink d) { return d == D; }, chosen};
//! [state]

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
      .on_clicked = [] { chosen = D; },
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
  {.id = "tiles", .style = lv::look<"tiles">},
  lv::group({.style = lv::look<"tile-row">}, tile<drink::espresso>(), tile<drink::lungo>()),
  lv::group({.style = lv::look<"tile-row">}, tile<drink::cappuccino>(), tile<drink::latte>())
);

constexpr auto ui = lv::group(
  {.style = lv::look<"page">}, header("CHOOSE A DRINK", 1, "1 / 4"), tiles,
  footer(no_back, primary("NEXT"))
);
//! [drinks]

// The display's memory: the screen of the token file, size.screen-width x size.screen-height.
constinit lv::frame<tok.size.screen_width->raw(), tok.size.screen_height->raw()> frame;

//! [geometry]
// The screen keeps the sizes of the design: each is a token, or a formula of tokens. The header
// is target high with the title in its middle, the footer target + space-4, and the content
// between them the rest. A size that does not follow the design is an exit code of its own.
int geometry(lv::obj screen, lv::mounted<ui>& root) {
  const auto width = tok.size.screen_width->raw(), height = tok.size.screen_height->raw();
  const auto header = tok.size.target->raw(), footer = header + tok.space._4->raw();
  const auto title = root->get<"title">();
  if (screen.get<"width">() != width || screen.get<"height">() != height) return 8;
  if (root->get<"header">().get<"height">() != header) return 4;
  if (std::abs(2 * title.get<"y">() + title.get<"height">() - header) > 1) return 4;
  if (root->get<"footer">().get<"height">() != footer) return 5;
  if (root->get<"tiles">().get<"height">() != height - header - footer) return 6;
  return 0;
}
//! [geometry]

// Whether the tile of D is the one tile filled.
template <drink D>
bool filled_alone() {
  const int filled = picked<drink::espresso>.get() + picked<drink::lungo>.get() +
                     picked<drink::cappuccino>.get() + picked<drink::latte>.get();
  return picked<D>.get() && filled == 1;
}

}  // namespace coffee

int main() {
  lv::application app;
  lv::display display{app, coffee::frame};
  auto root = lv::mount<coffee::ui>(display.screen());
  if (!root) return 1;
  for (int i = 0; i < 5; ++i) app.advance(10 * lv::ms), app.step();  // past the first refresh

  if (const int wrong = coffee::geometry(display.screen(), root)) return wrong;

  // The reader picks a lungo: its tile fills, and the espresso's empties. Then the espresso
  // again, as the mock-up shows it.
  coffee::chosen = coffee::drink::lungo;
  app.step();
  if (!coffee::filled_alone<coffee::drink::lungo>()) return 2;
  coffee::chosen = coffee::drink::espresso;
  app.step();
  if (!coffee::filled_alone<coffee::drink::espresso>()) return 2;

  return lv::write_png(display.screen(), "coffee_1.png") ? 0 : 3;
}
