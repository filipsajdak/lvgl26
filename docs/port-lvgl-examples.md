# Port LVGL's own examples

LVGL ships its examples in C, and lvgl26 rewrites them.
A test compares each port with the C original pixel for pixel, so a port cannot drift from what LVGL draws.

## One port

A port is one file with one function that builds the example on a screen.
This is the click counter of the button examples.

<!-- snippet: examples/lvgl/widgets/button_event.cpp clicks -->
```cpp
// How often the button was clicked. The label below it follows.
// The C example writes the label's text in its event handler. One difference: built
// a second time, the C example starts at "Clicks: 0" again while its count goes on;
// here the label always shows the count.
constinit lv::state clicks{lv::counter{}};

void count_click() {
  ++clicks;
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.flex_cross_place = "center",
                                         .pad_row = tok.gap.md}})[
    lv::button({.on_clicked = &count_click})[
        lv::label({.align = "center", .text = "Click me"})],
    lv::label({.text = lv::bind(clicks, "Clicks: {}")})];
```

A port that differs from the original says so in a comment at the top, as this one does.

## The same-pixels test

`tests/same_pixels.hpp` builds the C original on one fresh screen and the port on another.
It applies the same steps to both: a click, a drag, a new value, or some time.
Then it takes a picture of each and compares the pictures byte by byte.
A difference fails the test and prints the first pixel and the box around the differing pixels.

Each step is a case of its own.
The click counter is checked at rest, after one click and after two.
The tests are `tests/examples_get_started.cpp` and `tests/examples_widgets_*.cpp`, and the gate is `ctest --preset host-v96 -R examples_`.

## What is ported

| Group | Ported | Of | How a port is checked |
|---|---|---|---|
| LVGL `get_started` | 4 | 4 | Same pixels as the C original |
| LVGL `widgets`: label, button, arc, bar, slider, switch, checkbox, led, line, obj | 45 | 52 | Same pixels, also after clicks, drags and time |
| LVGL `widgets`: buttonmatrix, dropdown, roller, spinbox, textarea, image, scale, keyboard | 24 | 50 | Same pixels, also after a choice, a key or a click |
| Other `widgets`, `layouts`, `styles`, `event`, `anim`, `scroll` and the rest | 0 | 193 | Not started |

That is 73 of 299 examples.
[examples/README.md](../examples/README.md) holds the current table, and the list of what is not ported.

## A skipped example is a finding

Each skipped example is a gap in lvgl26 that stopped a port.
The port is left out, and no workaround is used.
The table "Not ported yet" in [examples/README.md](../examples/README.md) says what each one needs.

One finding shows the kind of gap.
A table has one `cell_value` in its properties, so the original's several calls that set a cell have no form.

For you, a finding is a warning.
If your UI uses the same feature, expect the same gap.
Look in the table before you start a port, and write your own gaps down, as the examples do.

Some widget families are not tried yet: lists, message boxes, tab views, tile views, menus, windows, calendars, charts, canvases and span groups.

## Rules for a port

- Styles use design tokens, and each example group has its own `tokens.json`.
- States are strongly typed: a measure has a unit, and a count is `lv::counter`.
- An example calls no C function of LVGL, and `tools/check_examples.py` fails when it does.
- The sources of Qt for MCUs are under a commercial licence, so they are not read into this repository.

To port a whole application from another framework, read [Port walkthrough: the Slint printer panel](port-slint-printer.md).
