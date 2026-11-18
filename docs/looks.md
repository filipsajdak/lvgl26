# Looks

A look is a named group of the project's design tokens, used as a style: `.style = lv::look<"tile">`. The code says what an element is, and the token file says how it looks.

The compiler finds the group in the token file, matches each entry to the style property of the same name, and builds a constant style in flash. A look costs no run time and no heap from lvgl26: LVGL keeps a pointer to the constant, as it does for every style of lvgl26.

![A printer's home screen: the title Printer, three tiles in blue, green and red that say Print, Scan and Copy, and the line Ready to print in green](media/looks.png)

## Name the token file once

A project names its token file once, before the first look, as it sets `lv::cfg`. The file with the specialization is included by every file of the project.

<!-- snippet: examples/looks/screen.hpp registration -->
```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto tok = lv::tokens<tokens_json>;
```

<!-- snippet: examples/looks/screen.hpp design -->
```cpp
template <>
inline constexpr lv::design_of<decltype(looks::tok)> lv::design<lv::override>{
    .tokens = looks::tok, .type = lv::typeface({{"Barlow", 500, looks::barlow}})};
```

`lv::design_of` holds the tokens and the typeface that draws the fonts of the typography tokens. A project whose looks have no typography token leaves the typeface out: `{.tokens = tok}`. A registration that comes after the first look is a compile error, as it is for `lv::cfg`.

## Write what each element is

<!-- snippet: examples/looks/screen.hpp tree -->
```cpp
constexpr auto tile = []<action Action> {
  return lv::button({.style = lv::look<"tile"> | lv::look_of<"tile", Action>})[
      lv::label({.text = lv::like<Action, names>})];
};

constexpr auto ui = lv::group({.style = lv::look<"page">})[
    lv::label({.style = lv::look<"title">, .text = "Printer"}),
    lv::group({.style = lv::look<"tiles">})[lv::for_each<action, lv::but<action::none>>(tile)],
    lv::label({.checked = true, .style = lv::look<"status">, .text = "Ready to print"})];
```

A look is a style value. It goes wherever a style goes: in a node, in a theme, in a part that takes an `lv::style_value`, and on either side of `|` with `lv::on(...)`, another style or a composite token. It is resolved when it is first used, and a file that names no look does not pay for any.

The token file holds one group for each look. Its entries are named for the style properties they set.

<!-- snippet: examples/looks/tokens.json -->
```json
{
  "$description": "The design of the looks example: values first, then one group for each look. A group's entries are named for the style properties they set.",
  "font": {"text": {"$type": "fontFamily", "$value": "Barlow"}},
  "text": {
    "$type": "typography",
    "body": {"$value": {"fontFamily": "{font.text}", "fontSize": "14px", "fontWeight": 500, "lineHeight": 1.2, "letterSpacing": "0px"}},
    "label": {"$value": {"fontFamily": "{font.text}", "fontSize": "16px", "fontWeight": 500, "lineHeight": 1.2, "letterSpacing": "0px"}},
    "title": {"$value": {"fontFamily": "{font.text}", "fontSize": "24px", "fontWeight": 500, "lineHeight": 1.2, "letterSpacing": "0px"}}
  },
  "color": {
    "$type": "color",
    "paper": {"$value": "#f1f5f9"},
    "ink": {"$value": "#1d293d"},
    "gray": {"$value": "#62748e"},
    "white": {"$value": "#ffffff"},
    "green": {"$value": "#00a63e"},
    "blue": {"$value": "#2b7fff"},
    "blue_deep": {"$value": "#155dfc"},
    "teal": {"$value": "#00bc7d"},
    "teal_deep": {"$value": "#009966"},
    "red": {"$value": "#ff2056"},
    "red_deep": {"$value": "#fd3d6d"},
    "ink_pressed": {"$value": "#314158"}
  },
  "space": {"$type": "dimension", "page": {"$value": "16px"}, "gap": {"$value": "12px"}},

  "page": {
    "bg_color": {"$value": "{color.paper}"},
    "bg_opa": {"$type": "number", "$value": 1},
    "layout": {"$type": "string", "$value": "flex"},
    "flex_flow": {"$type": "string", "$value": "column"},
    "pad_all": {"$value": "{space.page}"},
    "pad_row": {"$value": "{space.gap}"},
    "text_color": {"$value": "{color.ink}"},
    "text_font": {"$value": "{text.body}"}
  },
  "title": {
    "text_font": {"$value": "{text.title}"}
  },
  "tiles": {
    "layout": {"$type": "string", "$value": "flex"},
    "flex_flow": {"$type": "string", "$value": "row"},
    "pad_column": {"$value": "{space.gap}"},
    "width": {"$type": "dimension", "$value": "288px"},
    "height": {"$type": "dimension", "$value": "104px"}
  },
  "tile": {
    "width": {"$type": "dimension", "$value": "88px"},
    "height": {"$type": "dimension", "$value": "104px"},
    "radius": {"$type": "dimension", "$value": "16px"},
    "bg_opa": {"$type": "number", "$value": 1},
    "bg_grad_dir": {"$type": "string", "$value": "ver"},
    "shadow_width": {"$type": "dimension", "$value": "0px"},
    "layout": {"$type": "string", "$value": "flex"},
    "flex_flow": {"$type": "string", "$value": "column"},
    "flex_main_place": {"$type": "string", "$value": "center"},
    "flex_cross_place": {"$type": "string", "$value": "center"},
    "flex_track_place": {"$type": "string", "$value": "center"},
    "text_color": {"$value": "{color.white}"},
    "text_font": {"$value": "{text.label}"},
    "pressed": {"bg_opa": {"$type": "number", "$value": 0.8}},
    "print": {"bg_color": {"$value": "{color.blue}"}, "bg_grad_color": {"$value": "{color.blue_deep}"}},
    "scan": {"bg_color": {"$value": "{color.teal}"}, "bg_grad_color": {"$value": "{color.teal_deep}"}},
    "copy": {"bg_color": {"$value": "{color.red}"}, "bg_grad_color": {"$value": "{color.red_deep}"}}
  },
  "status": {
    "text_color": {"$value": "{color.gray}"},
    "checked": {"text_color": {"$value": "{color.green}"}}
  }
}
```

## What an entry may be

An entry is matched to the style property that has its name, and its token type is checked against what the property takes.

| Token type | Style properties | Example entry |
|---|---|---|
| `color` | A color: `bg_color`, `text_color`, `border_color` | `"bg_color": {"$value": "{color.paper}"}` |
| `dimension` | A length: `radius`, `width`, `pad_row`, `border_width` | `"radius": {"$type": "dimension", "$value": "16px"}` |
| `number`, 0 to 1 | An opacity: `bg_opa`, `text_opa` | `"bg_opa": {"$type": "number", "$value": 1}` |
| `number`, whole | A whole number: `arc_rotation` | `"flex_grow": {"$type": "number", "$value": 1}` |
| `duration` | A time: `anim_duration` | `"anim_duration": {"$type": "duration", "$value": "200ms"}` |
| `typography` on `text_font` | A font, drawn by the compiler through the typeface | `"text_font": {"$value": "{text.label}"}` |
| `string` | An LVGL value by its name, as `.align = "center"` is written in a tree | `"flex_flow": {"$type": "string", "$value": "column"}` |
| `shadow`, `border`, `transition` | Merged as `style \| token` is, under any name | `"elevation": {"$value": "{elevation.card}"}` |

A shorthand such as `pad_all`, `pad_gap` or `margin_hor` is kept as the properties it stands for, so that the style is a constant and a state can override one of them. `size` takes two values, and a token holds one: write `width` and `height`.

A look that sets `bg_color` does not set `bg_opa`. The look says `bg_opa` itself, once, in the group.

## Parts and states are layers

A nested group named like an LVGL state (`checked`, `pressed`, `focused`, `disabled`) or part (`knob`, `indicator`, `items`, `selected`) is a layer: the same constant style that `lv::on("knob", "pressed", {...})` makes. A state inside a part is both: `knob.pressed` is the pressed state of the knob.

```json
"gauge": {
  "bg_color": {"$value": "{color.gray}"},
  "indicator": {"bg_color": {"$value": "{color.blue}"}},
  "knob": {"bg_color": {"$value": "{color.white}"}, "pressed": {"bg_color": {"$value": "{color.red}"}}}
}
```

Any other nested group is a look of its own, and it is not part of its parent's look: `tile.print` is reached as `lv::look<"tile.print">`. The names of the parts and states are those of the LVGL in use, and `alt` is one of them: a variant called `alt`, `knob` or `focused` would be read as a layer, so give it another name. A layer holds values, parts and states only. A group of another name inside a layer is a compile error.

## Variants

The four tiles share a frame and differ in color. A variant is several looks in one string, and a later one wins:

```cpp
lv::button({.style = lv::look<"tile tile.print">})
```

`lv::look<"tile"> | lv::look<"tile.print">` means the same. The string form is shorter, and it names each look in one place. In a part that `lv::for_each` repeats, `lv::look_of<"tile", Action>` is the look named for the enumerator: `tile.print` for `action::print`. A name that the token file lacks is a compile error that lists the groups.

## Size and layout

A look may carry fixed sizes, padding, gaps, alignment and flex settings: LVGL keeps all of them as style properties. Positions that depend on other sizes, and percentages, stay in the tree: a token file cannot compute.

When a node and a look both say something, these rules decide:

- The node's own properties beat a look. LVGL's local styles beat an added style, and `.size`, `.flex_flow` and `.style_checked` are local. `lv::group({.size = {30 * px, 20 * px}, .style = lv::look<"tile">})` is 30 by 20.
- What a factory sets by default gives way to a look that says it. `lv::group` fills its parent, unless `.size` or its look sets a width or a height: a look with a `width` leaves the height to fill. `lv::row` and `lv::column` are flex containers, unless `.flex_flow` or the look sets the flow.
- A shorthand in a look does not beat a layer of the same look: both are constant styles, added in order.

Text style reaches a label by LVGL's own inheritance, from the look of its parent. The tile's `text_color` and `text_font` reach its label, so the label needs no look of its own. `text_color`, `text_opa`, `text_font`, `text_align`, letter and line spacing and `text_decor` are inherited; `image_recolor` is not, so an icon in a button needs a look of its own. A label that differs from its parent has a look of its own: `lv::look<"status">`.

## A part library with several designs

A part that several designs share takes the design as a template parameter and passes it on:

```cpp
template <class Design = lv::override>
constexpr auto chip = lv::box({.style = lv::look<"chip", Design>});
```

`chip<>` uses the design the project registered. `chip<dark>` uses a design registered for the tag `dark`: `lv::design<dark>`, specialized in the same way as `lv::design<lv::override>`.

## What a look costs

Nothing, where none is named. A file that includes `<lv/lv.hpp>` compiles in the same number of instructions with and without `lv/look.hpp`, and `tests/large_tree.cpp` is not dearer. A look is worked out when it is first used, once for each path and design.

| Measured, in instructions of the compiler | Cost |
|---|---|
| A look of five plain entries | about 0.6 G |
| One more layer, such as `pressed` | about 0.25 G |
| One more enumerator entry, such as `flex_flow` | about 0.2 G |
| One more typography entry (`text_font`) | about 6.6 G, against 3.0 G for `type(tok.x.text_font)` written in the tree: the compiler draws the font again for each look |
| 1, 10 and 50 looks of nine entries, one layer and a font, each in one box | 152.0 G, 211.2 G and 471.7 G |
| The same boxes with the styles written out | 146.8 G, 173.1 G and 289.9 G |

At run time a look is the constant `lv_style_t` and the property array that a written-out style makes. A box with two looks takes the same LVGL heap as a plain box, and a look with three layers takes 40 bytes more: LVGL's list of the styles of the widget. Built with optimization, no object of `lv::look` is left in the program. A project that wants fewer instructions names a typography token in fewer looks, and lets the label inherit the text of its parent.

## Mistakes

Each one is a compile error with a message that says what is valid.

| Mistake | The message says |
|---|---|
| `lv::look<"tile.pritn">` | There is no group `pritn` in `tile`; the groups there: `inner`, `print`, `scan`; did you mean `print` |
| `lv::look<"tile.radius">` | `tile.radius` is a token, not a group |
| `lv::look<"">` | The look names no look; the groups of the token file |
| An entry `colour` | It fits no style property; the nearest ones |
| `radius` as a color | The entry is a color, the property takes a dimension |
| `"text_align": "centre"` | LVGL has no such value; its values |
| A group that is no part or state, inside a state | It is neither; the parts and the states |
| No registration | Name the token file once, before the first look |

In an editor, Clang marks a look that names no group of the token file where it is written ([editor support](editor-support.md)). It does not check the entries against the style properties: GCC does that.

## Find the looks that nothing uses

A token file grows, and a group that no element names is dead weight. `tools/look_groups.py` lists the component groups of a token file that no source names in an `lv::look`:

```sh
python3 tools/look_groups.py tokens.json src/
```

A component group is a group with an entry named for a style property. Groups of values (`color`, `space`, `text`) have none, and are never listed. A group named like a state or a part belongs to its parent. `lv::look_of<"tile", E>` uses every group directly under `tile`, since the compiler picks one by the name of an enumerator. A token file may list its components itself with `"$extensions": {"lvgl26": {"components": ["tile", "status"]}}`. A group that another program uses is passed with `--ignore`. The test `look_groups_example` runs it on this example, and a misspelled `status.cheked` is listed, because no look reaches it.

## The same screen, written out

`examples/looks/before.hpp` is the screen above with every design value named in the tree. Both draw the same pixels: the test `looks_example` compares them.

<!-- snippet: examples/looks/before.hpp before -->
```cpp
constexpr lv::typeface type({{"Barlow", 500, barlow}});

constexpr auto tile(const char* text, const auto& tint) {
  const auto& t = tok.tile;
  return lv::button({.flex_align = {"center", "center", "center"},
                     .flex_flow = "column",
                     .style = {.bg_color = tint.bg_color,
                               .bg_grad_color = tint.bg_grad_color,
                               .bg_grad_dir = "ver",
                               .bg_opa = t.bg_opa,
                               .height = t.height,
                               .radius = t.radius,
                               .shadow_width = t.shadow_width,
                               .text_color = t.text_color,
                               .text_font = type(t.text_font),
                               .width = t.width},
                     .style_pressed = {.bg_opa = t.pressed.bg_opa}})[lv::label({.text = text})];
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.bg_color = tok.page.bg_color,
                                         .bg_opa = tok.page.bg_opa,
                                         .pad_all = tok.page.pad_all,
                                         .pad_row = tok.page.pad_row,
                                         .text_color = tok.page.text_color,
                                         .text_font = type(tok.page.text_font)}})[
    lv::label({.style = {.text_font = type(tok.title.text_font)}, .text = "Printer"}),
    lv::group({.flex_flow = "row",
               .style = {.height = tok.tiles.height, .pad_column = tok.tiles.pad_column, .width = tok.tiles.width}})[
        tile(names::print, tok.tile.print), tile(names::scan, tok.tile.scan), tile(names::copy, tok.tile.copy)],
    lv::label({.checked = true,
               .style = {.text_color = tok.status.text_color},
               .style_checked = {.text_color = tok.status.checked.text_color},
               .text = "Ready to print"})];
```

The words of the code, counted as `tools/code_words.py` counts them (an identifier, a number or a string; no punctuation, no comments):

| | Written out | With looks |
|---|---|---|
| Code that says how the screen looks | <!-- words: examples/looks/before.hpp before 165 -->165 | <!-- words: examples/looks/screen.hpp design 22 -->22 for the registration, once for the project, and <!-- words: examples/looks/screen.hpp tree 64 -->64 for the tree |
| Token entries | 36 | 36 |

The words moved. In this example the token file is the same in both columns: the entries are named for style properties either way, and the written-out code reads them one by one. A project that keeps only semantic tokens today (`color.print`, `space.gap`) has to write the component entries the looks need, and then the token file grows by about what the code loses. For the printer port of the research note `docs/research/less-boilerplate.md`, the code loses about 900 words and the token file gains about 600. A look pays off where one group is used again: the tile look serves three tiles here, and the spin box look of that port serves two pages.
