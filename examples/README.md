# Examples

Existing UIs rewritten with lvgl26. Every port has a test, and every port is built
with all the quality presets.

| Group | Ported | Of | How a port is checked |
|---|---|---|---|
| LVGL `get_started` | 4 | 4 | Same pixels as the C original (`tests/examples_get_started.cpp`) |
| LVGL `widgets`: label, button, arc, bar, slider, switch, checkbox, led, line, obj | 48 | 52 | Same pixels as the C original, also after clicks, drags and time (`tests/examples_widgets_*.cpp`) |
| LVGL `widgets`: buttonmatrix, dropdown, roller, spinbox, textarea, image, scale, keyboard | 30 | 50 | Same pixels as the C original, also after a choice, a key or a click (`tests/examples_widgets_*.cpp`) |
| LVGL other `widgets` (spinner so far), `layouts`, `styles`, `event`, `anim`, `scroll`, others | 1 | 193 | Same pixels as the C original, also after time (`tests/examples_widgets_spinner.cpp`) |
| Qt for MCUs demos and examples | 0 | 35 | Screenshots; written from the documented behaviour only |
| Qt 6 and Qt Design Studio examples (BSD-3-Clause) | 1 | about 60 | Screenshots; [treadmill](ports/qt/treadmill/README.md) is the Qt Design Studio port (`tests/ports_treadmill.cpp`) |

## Not ported yet

[docs/research/ported-examples.md](../docs/research/ported-examples.md) lists each widget example that is not ported, and what it needs.
Of LVGL v9.6.0's 163 widget examples, 79 are ported. The other 84 wait for a form that lvgl26 does not have yet:

| What it needs | Examples |
|---|---|
| A child made by an `add_` function: a tab, a chart series, a menu page, a list button, a message box's title, a span, a scale section, a calendar header, a tile | 36 |
| Drawing on a canvas layer, or in a draw-task event | 16 |
| One setter called several times: a table's cells, a control or a width for each button | 11 |
| The order of setters: a button matrix's controls run before `one_checked`, which clears them (pending two rules of the setter table) | 2 |
| A call that is no setter: `lv_animimg_start`, `lv_textarea_add_text`, `lv_spinbox_increment` | 5 |
| An LVGL build option that the test build leaves off: Lottie, SVG, a transform matrix | 5 |
| Another widget as a pointer: a keyboard's text area, a scale's needle | 3 |
| A feature of its own: translations, the input device's motion, a style bound to a state, a style transition, the dropdown's list, a percentage as a whole number | 6 |

## Findings

Each finding is a gap in lvgl26 that stopped a port. The port is left out and no workaround is used.

A table has one `cell_value` in its props, so the C original's several `lv_table_set_cell_value` calls have no form:

```cpp
lv::widget<"table">({.cell_value = {0, 0, "City"}})  // and the cell (0, 1), (1, 0) ... ?
```

A control map is an array of enumerators, and there is no way to write them by name:

```cpp
constexpr lv_buttonmatrix_ctrl_t ctrl[] = {"none", "checkable"};  // wanted
```

The theme gives a screen a gap between its children.
An example that lays out the screen itself (a row or a column with `space_evenly`) needs that gap.
Its port sets the layout on the screen with `screen.set({...})`, as the `switch_orientation` port does.
A group in the screen's place has no gap, and the pictures differ.

## Rules

- The examples keep the framework's defaults: styles use design tokens, states are
  strongly typed. Each example group has its own `tokens.json`.
- A port that differs from the original says so in a comment at the top of the file.
  It names there each call of the C example that it makes in another order:
  a node sets its properties in the order of their names, and LVGL needs some
  calls in an order of its own.
- Qt for MCUs sources are under a commercial licence: they are not read into this
  repository and no asset is copied from them.

Found while porting the treadmill panel of Qt Design Studio ([examples/ports/qt/treadmill](ports/qt/treadmill/README.md)).
Each is a gap in lvgl26, and the port leaves the feature out or says how it differs.

The opacity of a widget cannot be animated, because LVGL has no setter for it (`lv_obj_set_style_opa` is a style).
The original fades its splash screen and its logos in and out:

```cpp
co_await lv::animate(page, {.opa = {100 * lv::pct, 0 * lv::pct}}, 1000 * lv::ms);  // no member `opa`
```

A style cannot follow a state, so a look that changes while the program runs is a new tree.
The original changes three themes of colors in place:

```cpp
lv::group({.style = {.bg_color = lv::bind(background)}})  // a style takes a token, not a binding
```

A position cannot follow a state of a plain number. A binding to `.x` accepts only a length, and no state holds one:

```cpp
struct place { int value; };
constinit lv::state<place> where{place{0}};
lv::image({.x = lv::bind(where)})  // this property cannot be bound to a state of that type
```

A text that has two numbers in it (`6:05`, `14.3`) has no formatted binding, because a format holds one `{}`.
The port writes the text in an effect and binds the label to a `lv::state<const char*>`.
