# A display that follows a state: live theme switching

Status: a design for the owner's decision of 2026-11-27 (gap 6 of the coffee v2 plan: the display follows `lv::state<lv::signal::mode>`, setting it restyles every widget in place). Nothing is built. Marks: `source` is read in this repository or in LVGL v9.6.0 and the v10 pin, `reasoned` is mine.

## How a theme reaches a widget today

- `lv::theme({...})` makes a constant list of `theme_entry {widget class, selector, const lv_style_t*}` (`include/lv/detail/theme_styles.hpp`). `lv::display` makes one `lv_theme_t` whose parent is LVGL's own theme, whose `apply_cb` is `detail::apply_theme`, and whose user data is the first entry (`display::use`). `source`
- LVGL calls the `apply_cb` once for each **new** widget, inside its creation. `apply_theme` adds the entries of the widget's class (a screen is a widget with no parent) with `add_style`. A widget that exists already is not touched when the theme changes. `source`
- A node's own `.style`, a look and the start style of a factory are added **after** the theme, so they win over it. A factory that removes all styles of its widget (`lv::group`, `lv_obj_remove_style_all`) removes the theme's styles too and adds `start_style` back (CLAUDE.md): a group holds no style of the theme. `source`

## How LVGL can restyle a widget in place

- `lv_obj_replace_style(obj, old_style, new_style, selector)` is in v9.6.0 and in the v10 pin. It finds the widget's entry for `old_style` with that part and state and puts `new_style` there: **the position among the widget's styles stays**, so a look or a node style that was added after the theme still wins over it. It allocates nothing (it overwrites an entry of the style list), drops running transitions of the part, invalidates the widget when the draw area can change, and refreshes the style of the part. It returns false when the widget does not hold `old_style` (a group) or when both styles are the same pointer. `source`
- Removing the old entry and adding the new would free and allocate heap and put the new style last in the list: wrong for both rules (no heap, order).
- `lv_obj_report_style_change(nullptr)` refreshes every widget of every display: not needed, since replace refreshes what it changes.
- Cost of a switch: one walk of the tree; for each widget, a loop over the theme's entries of its class (a few) and a pointer search in its style list. No allocation. Each changed widget is invalidated, so the next frame draws the screen again. For the 112-widget tree of `tests/large_tree.cpp`, that is some hundreds of replaces. `reasoned`

## The watch

The follow uses a `detail::watch_node` that the display holds and appends to the state's list (`state::follow()`): nothing is allocated, no LVGL observer and no subject (CLAUDE.md, "Something follows an lv::state ..."). The node's `changed` reads the state, takes the theme of that value and swaps. The display's destructor removes its node; the state's destructor detaches all nodes. `source` for the mechanism, `reasoned` for the use.

## What a switch does

1. `old` is the theme entry list that the widgets hold now, `new` the one of the new value. Both must have the **same shape**: the same widget classes and selectors in the same order (Signal's `day` and `dark` do: both are `themed<Tokens>()`). Entry i of `old` is replaced by entry i of `new` in every widget of that class.
2. The walk covers the active screen and the three layers of the display (bottom, top, system), depth first. A screen that is not loaded keeps the theme it was made with; it is not walked (a limit, documented).
3. `lv_theme_set_user_data(theme, new first entry)`: a widget made after the switch gets the new theme from `apply_theme`, as before.
4. Fonts: a theme's `text_font` is part of its styles. Signal's `day_with_type` and `dark_with_type` share `body_font` and `label_font`, so the replace changes colors and keeps the fonts. A theme pair with different fonts changes them too, and the layout follows.
5. A mismatch of shape is a contract violation where the display adopts the themes (run time, once): comparing the classes of two constant lists in a constant expression is not reliable under the sanitizers (CLAUDE.md, GCC 16.2). `reasoned`

## The API: three forms

**A (my proposal): the display is made with it.** `lv::themed(state, {theme, theme, ...})` returns what a display takes today, a `theme_styles`, which now can also carry the follow. The themes are indexed by the enumerators of the state's enum in order, as `lv::options_of<E>` lists them; one theme for each enumerator is a `static_assert`. The existing constructors of `lv::display` stay as they are.

```cpp
constinit lv::state<lv::signal::mode> mode{lv::signal::mode::day};

lv::display display{app, frame, lv::themed(mode, {lv::signal::day_with_type, lv::signal::dark_with_type})};
// ...
mode = lv::signal::mode::dark;   // every widget of the display changes in place

lv::dropdown({.options = lv::options_of<lv::signal::mode>, .selected = lv::bind(mode)})   // the Service row
```

`themed` is `consteval`: the state is a static object, so its address is a constant; the table is stored with `std::define_static_object`. `lv::signal::mode` is a new enum `{day, dark}` in `lv/signal.hpp`.

**B: pairs of a value and a theme.** `lv::themed(mode, {{mode::day, day}, {mode::dark, dark}})`: explicit, any order, a value may share a theme. More to write, no dependence on the order of the enumerators. Same type of result.

**C: a method of the display.** `display.follow(mode, {day, dark})` after the display is made: one new member function (a template) and no change of `theme_styles`, but the display is made without a theme and then given one, and `use` runs twice.

A and B need `theme_styles` to carry a pointer to a constant record (the state, a reader of its index, the table of themes), 8 bytes more in a type that the display constructors take by value. C needs nothing there. The record is what the display's follow node reads.

## What does not follow

- A node's own `.style` with a token (`tok.color.surface` of one token file) is constant: it does not change with the mode. Only what a **theme** gives follows. A screen that takes all its colors from the theme (a box, a button, an arc, a bar, the screen, inherited text) follows entirely.
- A look from the project's tokens (`lv::look<"tile">`) is one constant style per path of one design. It does not follow the mode. A mode-aware look would be a pair of constant styles, one for each design, replaced like the theme's entries; it needs a registry of the pairs that a switch can search (no list of looks exists), so it is a separate feature if wanted.

## Tests I would write

A tree of boxes, buttons and arcs mounted under a display with `themed`, checked by the colors that LVGL reports for each widget before and after the switch (every widget, including a nested one and one made after the switch); a group keeps its start style; a node's own style and a look still win over the theme after the switch; no heap change over a switch (`lv_mem_monitor`); a display deleted while the state lives, and a state deleted before the display; a theme pair of different shapes ends in a contract violation; compile-fail tests: a count of themes that is not the count of enumerators, a state that is not an enum, a theme list in the wrong shape.

## Open (asked of the team lead)

1. Form A, B or C? (I propose A.)
2. Is it enough that only what a theme gives follows the mode? The coffee example would then take every color of its screens from the Signal theme (no `tok.color.*` in a node's style), or the mock-up needs mode-aware looks, a second feature.
3. A screen that is not loaded keeps its old theme until the application makes it again: acceptable?
4. Two themes of one display must have the same shape: a contract violation at the display's construction if not. Acceptable, or must a switch also handle themes of different shapes (then it adds and removes styles: allocation, and the theme styles move to the end of the list)?
