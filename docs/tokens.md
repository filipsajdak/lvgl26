# Design tokens

Design tokens come from a Design Tokens Format file that the compiler reads, and a style takes them as typed constants.

Header: `<lv/lv.hpp>`, the core.

## Read a token file

```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

lv::box({.style = {.bg_color = tok.color.surface, .radius = tok.radius.card}})
```

- Each group of tokens becomes a struct, and each token a typed `lv::token<T>`.
- The types are colors, dimensions (spacing, radius, sizes), durations, numbers (opacity), font family and weight, and easing curves.
- The composite types are shadow, border, typography, transition and gradient.
- An alias is resolved by the compiler.
- A misspelled token, a broken alias and a value that cannot be read are compile errors.

The forms a token file is written in, and the tools that check and convert one, are in [Editor support](editor-support.md#design-token-files).

## Styles take tokens

A design value in a style must be a token: a color, a length, an opacity, a time, a number.

```cpp
.style = {.radius = tok.card.radius}                            // ok
.style = lv::style({...}) | tok.card.border | tok.card.shadow  // composite tokens merge with |
.style = {.radius = lv::raw(3 * px)}                           // a deliberate, searchable exception
.style = {.radius = 8 * px}                                    // error: it takes a design token
```

The last line stops the build:

<!-- message: style_needs_token -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a length (120 * lv::px, 50 * lv::pct or lv::size_content). You gave lv::length. A bare value is no token: write lv::raw(...) around it, or take it from the token file.
```

Layout values, such as `.size`, `.range` and `.value`, and LVGL's names stay free.

To accept plain values in styles, change the project setting in the [settings file](settings.md):

```json
{"rules": {"raw_style_values": true}}
```

## Three tiers

Keep tokens in three tiers:

1. primitive values, such as `primitive.color.blue-500`;
2. semantic roles that alias them, such as `color.primary`;
3. component tokens that alias those, such as `card.radius`.

UI code uses the last two.

## Where a style lives

A style in a mounted tree is a constant `lv_style_t` in read-only memory. LVGL keeps a pointer to it.

A style for one part in one state, `lv::on("knob", "pressed", {...})`, is one more constant style. It costs no heap beyond LVGL's entry in the widget's list of styles. [Themes](themes.md#style-a-part-in-a-state) shows it, and [Looks](looks.md) builds such styles from groups of tokens.
