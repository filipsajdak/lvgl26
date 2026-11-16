# Themes

A theme gives every widget of a kind its look, so a screen needs no styles of its own. Signal comes with lvgl26, and you make your own from a token file.

| Signal, dark | A theme of your own |
|---|---|
| ![The screen in Signal: black ground, a lime ring at 72 percent and a square lime button](media/theme-signal.png) | ![The same screen in a paper look: cream ground, a red ring and a rounded red button](media/theme-paper.png) |

Both pictures show this screen. It has no style in it.

<!-- snippet: examples/theme/main.cpp tree -->
```cpp
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}})[
    lv::label({.text = "Battery"}),
    lv::arc({.range = {0, 100}, .size = {110 * px, 110 * px}, .value = 72})[
        lv::label({.align = "center", .text = "72 %"})],
    lv::button[lv::label({.text = "Charge"})]];
```

## Use Signal

Signal is the design system of this project. Include it and give it to the display:

```cpp
#include <lv/signal.hpp>

lv::display display{app, frame, lv::signal::dark};
```

- `lv::signal::dark` is for indoor devices and OLED screens. `lv::signal::day` is for screens read in sunlight.
- The theme covers the screen, boxes, buttons, arcs and bars. The other widgets keep LVGL's look.
- Signal's tokens are there for your own styles: `lv::signal::dark_tokens.color.muted`.

## Use Signal's typefaces

Signal sets numbers and titles in Barlow Condensed and everything else in Barlow. A second header makes these fonts, and two themes that use them.

![Signal with its typefaces. The title LIVING ROOM, the reading 21.5 degrees in large condensed digits, a line of text and a lime button that says HOLD](media/theme-signal-type.png)

```cpp
#include <lv/signal_type.hpp>

lv::display display{app, frame, lv::signal::dark_with_type};
```

- `lv::signal::dark_with_type` and `lv::signal::day_with_type` are `dark` and `day` with Signal's fonts.
- All text is `body`: Barlow at 16 px. The text of a button is `label`: Barlow Condensed at 16 px.
- Text is inherited. A label in a box takes the font of the screen, and a label in a button takes the font of the button.
- The two fonts are `lv::signal::body_font` and `lv::signal::label_font`, for a style of your own.

The larger styles are yours to make, once each, in the file that shows them. `lv::signal::type` is Signal's typeface, and `lv::signal::type_tokens` is its typography.

<!-- snippet: examples/signal_type/main.cpp fonts -->
```cpp
// A copy: a reference to the tokens would keep the token file in the program.
constexpr auto text = lv::signal::type_tokens.text;

constexpr auto title = lv::signal::type(text.title);            // 24 px, the space to the tilde
constexpr auto reading = lv::signal::numerals(text.numeral_l);  // 72 px, the sixteen characters of a reading
```

<!-- snippet: examples/signal_type/main.cpp tree -->
```cpp
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"},
                                .size = {320 * px, 240 * px},
                                .style = {.pad_gap = lv::signal::dark_tokens.space._2}})[
    lv::label({.style = lv::style({.text_font = title}) | text.title, .text = "LIVING ROOM"}),  // and its spacing
    lv::label({.style = {.text_font = reading}, .text = "21.5°"}),
    lv::label({.text = "Heating to 22 degrees."}),
    lv::button[lv::label({.text = "HOLD"})]];
```

- `type` makes a font with the characters from the space to the tilde.
- `numerals` makes a font with sixteen characters: the digits, the point, the colon, the minus, the percent sign, the degree sign and the space. Signal keeps its large numbers in such a font.
- `numerals` holds Barlow Condensed at weight 700 only. A token of another style stops the build.

### What the typefaces cost

Measured once on one machine, with `-Os` for the flash. The compile time comes on top of `lv/signal.hpp`.

| Font | Characters | Compile time | In flash |
|---|---|---|---|
| `body` and `label`, 16 px: what the header makes | 95 each | about 7 s | 13 KB |
| `type` at `title`, 24 px | 95 | about 4 s | 11 KB |
| `numerals` at `numeral-m`, 40 px | 16 | under 1 s | 4 KB |
| `numerals` at `numeral-l`, 72 px | 16 | about 2 s | 11 KB |
| `numerals` at `numeral-xl`, 104 px | 16 | about 3 s | 20 KB |
| `numerals` at `numeral-2xl`, 208 px | 16 | about 12 s | 75 KB |
| `type` at `numeral-l`, 72 px | 95 | about 11 s | 62 KB |
| `type` at `numeral-xl`, 104 px | 95 | about 21 s | 123 KB |

- This cost is why the fonts are not in `lv/signal.hpp`. Include `lv/signal_type.hpp` in one file, and make each larger font once.
- `title` and `label` are capitals with wide spacing. The themes space the text of a button; for a style of your own, merge the token: `lv::style({.text_font = title}) | text.title`.
- A numeral style with all 95 characters takes six times the flash of `numerals`. Use `type` for it only when the reading has letters.
- `numeral-2xl` is 208 px, and `numerals` makes it. `type` does not: 95 characters of that size need more compile-time work than lvgl26 allows. The build stops at GCC's `-fconstexpr-ops-limit`.
- The fonts have no glyph for LVGL's symbols. A widget that draws one, such as the arrow of a drop-down list, shows an empty box in its place.

## Make your own

A theme is one constant. It has a member for each kind of widget, and each member holds the styles a node of a tree can have: `style`, `style_pressed`, `style_indicator` and the rest.

<!-- snippet: examples/theme/main.cpp paper -->
```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto paper = lv::theme({
    .arc = {.style = {.arc_color = tok.color.rule, .arc_width = tok.stroke.ring},
            .style_indicator = {.arc_color = tok.color.accent, .arc_width = tok.stroke.ring},
            .style_knob = {.bg_opa = tok.opacity.none}},
    .button = {.style = {.bg_color = tok.color.accent,
                         .pad_hor = tok.space.button_x,
                         .pad_ver = tok.space.button_y,
                         .radius = tok.radius.button,
                         .shadow_width = tok.space.none,
                         .text_color = tok.color.on_accent},
               .style_pressed = {.bg_color = tok.color.ink}},
    .obj = {.style = {.bg_opa = tok.opacity.none,
                      .border_width = tok.space.none,
                      .pad_all = tok.space.edge,
                      .pad_gap = tok.space.gap,
                      .text_color = tok.color.ink}},
    .screen = {.style = {.bg_color = tok.color.paper, .bg_opa = tok.opacity.opaque}},
});
```

The values are design tokens, as in every style. The token file of this example is [examples/theme/tokens.json](../examples/theme/tokens.json).

| Member | Styles |
|---|---|
| `screen` | A widget without a parent |
| `obj` | A plain box: `lv::box`, `lv::column`, `lv::row` |
| `arc`, `bar`, `button`, `label`, ... | The widget LVGL calls by that name |
| `toggle` | LVGL's switch: `switch` is a C++ keyword |

The members are in alphabetical order, as designated initializers need.

## Give it to a display

The theme is the last argument of a display. This function draws the screen with any theme:

<!-- snippet: examples/theme/main.cpp draw -->
```cpp
bool draw(lv::application& app, lv::theme_styles theme, const char* file) {
  lv::display display{app, frame, theme};
  auto root = lv::mount<ui>(display.screen());
  return root && lv::write_png(display.screen(), file);
}
```

<!-- snippet: examples/theme/main.cpp main -->
```cpp
int main() {
  lv::application app;
  if (!draw(app, lv::signal::dark, "theme_signal.png")) return 1;
  if (!draw(app, paper, "theme_paper.png")) return 2;
  return 0;
}
```

## What a theme costs

- **No heap.** The compiler works the theme out, and its styles are constants in read-only memory. LVGL keeps a pointer to each.
- **Compile time.** A file that makes a theme compiles about a second longer. `lv/signal.hpp` makes two themes: about two seconds. Include it in one file.

## Rules

- A theme is applied after LVGL's own theme. It only says what differs.
- A node still has the last word: its `.style` wins over the theme.
- A widget gets the styles of its own kind only. A slider does not get those of `bar`.
- A member styles one part or one state. A part in a state (the indicator of a checked switch) is not there yet.
- A display takes its theme when it is made. It cannot change it later.
