# Writing good lvgl26 code

The rules that make lvgl26 code short, fast to run, fast to compile, checked by the compiler, clear and easy to grow.

<!-- Written by tools/rules.py from docs/reference/rules.json. Change the data, then run: python3 tools/rules.py --page docs/rules.md -->

Each rule has an id that never changes. A guide ends with the rules of its feature, and a review or a commit can cite one by its id.

Every example is a part of a file that the tests build. A message is the one the compiler prints, as its compile-fail test pins it.

Where two qualities pull apart, a rule says which one wins, and why.

| Id | Rule |
|---|---|
| [`no-boilerplate-1`](#no-boilerplate-1) | Write a screen as one constant tree, `constexpr auto ui = lv::column[...]`, and mount it; never create its widgets one by one. |
| [`no-boilerplate-2`](#no-boilerplate-2) | Make a property follow a state with `lv::bind`, instead of a handler that copies the value into the widget. |
| [`no-boilerplate-3`](#no-boilerplate-3) | Write a handler that only changes a state as a lambda in the tree: `.on_clicked = [] { ++cups; }`. |
| [`no-boilerplate-4`](#no-boilerplate-4) | Show rows that change with `lv::each` over an `lv::list`, and change the list, never the widgets. |
| [`no-boilerplate-5`](#no-boilerplate-5) | Write a set that is fixed when the program is written with `lv::for_each`: a part for each enumerator, captioned member or constant record. |
| [`no-boilerplate-6`](#no-boilerplate-6) | Give a dropdown or a roller its options as a list, `{"Cut", "Copy"}`, or as `lv::options_of<E>`, never as one text with line breaks. |
| [`no-boilerplate-7`](#no-boilerplate-7) | Give all widgets of a kind their look in a theme, not in a style on each node. |
| [`no-boilerplate-8`](#no-boilerplate-8) | Start the program with `lv::application`, take the screen from an `lv::display`, and end `main` with `return app.run();`. |
| [`no-boilerplate-9`](#no-boilerplate-9) | Switch the pages of a tree with `lv::shown(state)`, and one of several dialogs at a root with an `lv::one_of` slot; never write a pager. |
| [`fast-run-1`](#fast-run-1) | Keep states, lists and mounted trees in `constinit` globals or in variables: lvgl26 takes no heap for them. |
| [`fast-run-2`](#fast-run-2) | Change one row with `insert`, `erase`, `move` or `update`; never clear and fill a list again to change it. |
| [`fast-run-3`](#fast-run-3) | Show a text field of a row through `lv::bind`; do not copy it into a buffer of your own. |
| [`fast-run-4`](#fast-run-4) | Start work that belongs to a part of the screen with `lv::spawn(task, owner)`, so that it ends when the owner is deleted. |
| [`fast-run-5`](#fast-run-5) | Change a mounted widget with `set({...})` written in place, and give it a named style or a look rather than `lv::style({...})` built in the call. |
| [`fast-run-6`](#fast-run-6) | Let the compiler make the fonts and pictures known at build time; decode on the device only photos and sizes chosen at run time. |
| [`fast-run-7`](#fast-run-7) | Turn off what the product does not use: `LVGL26_NO_DEFAULT_THEME` when your theme covers every widget, and the options of the decoders. |
| [`fast-run-8`](#fast-run-8) | When nobody sees the screen, say so with `display.hide()`: a display that is not visible draws nothing until it is shown again. |
| [`fast-run-9`](#fast-run-9) | Hide a widget that nobody should see with `.hidden`; do not cover it or make it transparent. |
| [`fast-run-10`](#fast-run-10) | Make a property follow a state through `lv::bind`; never poll a value in a timer or a loop to set the property. |
| [`fast-run-11`](#fast-run-11) | Change only what changed: set or bind the one property, and never mount a tree again to show a new value. |
| [`fast-run-12`](#fast-run-12) | Animate the smallest widget that shows the motion, and let the animation end when the motion is over. |
| [`fast-build-1`](#fast-build-1) | Include `<lv/lv.hpp>` and the header of each feature the file uses; include `<lv/all.hpp>` only in a first program. |
| [`fast-build-2`](#fast-build-2) | Name drawn fonts in few files, and give the other files their text through the display's theme. |
| [`fast-build-4`](#fast-build-4) | Let the compiler decode pictures up to the limit in the settings, and decode larger ones on the device with `lv::encoded_image`. |
| [`fast-build-5`](#fast-build-5) | Bind, name and style nodes freely: after its first use in a file, a binding, an id or a property costs little to compile. |
| [`checked-1`](#checked-1) | Give every value a type that says what it is: a quantity, an `lv::counter`, or a struct of one member. |
| [`checked-2`](#checked-2) | Write an LVGL enumerator as its name in quotes, `.align = "center"`; a misspelled name stops the build and lists the right ones. |
| [`checked-3`](#checked-3) | Give a style design tokens, and mark a value that no token should name with `lv::raw(...)`. |
| [`checked-4`](#checked-4) | Name one settings file for the program, and forbid in it what the product may not use: allocation, threads, files, floating point. |
| [`checked-5`](#checked-5) | Handle the `std::expected` that `lv::mount`, `insert` and `assign` return: a full list or a heap without room is a value, not a crash. |
| [`checked-6`](#checked-6) | Read the first error and its `note:` lines before anything else, and do what the message says. |
| [`checked-7`](#checked-7) | Keep lvgl26's headers out of a precompiled header. |
| [`checked-8`](#checked-8) | Reach LVGL's C API only through `raw()`, and only for what lvgl26 has no form for; never to quiet an error. |
| [`checked-9`](#checked-9) | Reach a widget of a mounted tree by its id, `root->get<"title">()`; never keep a pointer to it or search for it at run time. |
| [`expressive-1`](#expressive-1) | Write a choice as an enum, and let the compiler derive from it: `lv::options_of<E>` for its options, `lv::match` for one arm per value. |
| [`expressive-2`](#expressive-2) | Name a value made from states as an `lv::computed`, and bind it like a state: `.hidden = lv::bind(enough)`. |
| [`expressive-3`](#expressive-3) | Write a sequence in time, such as wait for a click, pause, then animate, as one `lv::task` coroutine, not as callbacks and timers. |
| [`expressive-4`](#expressive-4) | Name the look of an element, `lv::look<"tile">`, and keep its values in the token file. |
| [`expressive-5`](#expressive-5) | Use semantic and component tokens in screen code, such as `tok.color.selected`, not primitives such as a step of a palette. |
| [`expressive-6`](#expressive-6) | Build a screen from named pieces: small constants, each at most about three levels deep, that the screen lists. |
| [`extendable-1`](#extendable-1) | Put one screen in each file with the states only it uses, and give shared state, parts, design, platform and `main` places of their own. |
| [`extendable-2`](#extendable-2) | Make a reusable piece of screen a function that returns a tree, and constrain each parameter with a public concept: `lv::dependency`, `lv::tree_handler`, `lv::tree_node`. |
| [`extendable-3`](#extendable-3) | Give a part its state as a template argument, `stepper<cups>(...)`, so that its handlers can name the state. |
| [`extendable-4`](#extendable-4) | Keep the design in a token file named once for the project, and let a library of parts take the design as a template parameter. |
| [`extendable-5`](#extendable-5) | Connect a platform as an object that feeds `lv::display` and `lv::pointer` and joins the loop with `lv::in_loop`; the application names no platform. |
| [`extendable-6`](#extendable-6) | Reach a widget that has no short name with `lv::widget<"spinbox">`: its properties and events come from LVGL's headers, as for every widget. |

## No boilerplate

Say what the screen is, and let lvgl26 write the code that builds it, keeps it current and takes it down.

### no-boilerplate-1

**Write a screen as one constant tree, `constexpr auto ui = lv::column[...]`, and mount it; never create its widgets one by one.**

The compiler checks the whole tree against the widgets' properties, and `lv::mount` builds every widget and deletes them all when it ends.

<!-- snippet: examples/state/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::dropdown({.options = lv::options_of<drink>, .selected = lv::bind(chosen), .width = 140 * px}),
  stepper<cups>(cups_text),
  lv::slider({.range = {1, 5}, .value = lv::bind(bold), .width = 200 * px}),  // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
```

Checked: no tool checks this rule yet.

Guide: The tree and its widgets (not written yet).

### no-boilerplate-2

**Make a property follow a state with `lv::bind`, instead of a handler that copies the value into the widget.**

A binding sets every widget that shows the value after each change, and an arc, a slider or a dropdown writes the user's change back.

<!-- snippet: examples/state/main.cpp smallest -->
```cpp
constinit lv::state cups{lv::counter{2}};  // a value that widgets follow

constexpr auto cups_text = lv::label({.text = lv::bind(cups, "{} cups")});  // shows it, and follows it
```

Checked: no tool checks this rule yet.

Guide: [State and bindings](state.md).

### no-boilerplate-3

**Write a handler that only changes a state as a lambda in the tree: `.on_clicked = [] { ++cups; }`.**

A state takes `=`, `++`, `--`, `+=` and `-=`. A named function, a pointer to it or an `update` call adds lines and says nothing more.

<!-- snippet: examples/state/main.cpp part -->
```cpp
// A count between two buttons. The part names its state as a template argument: a handler in a
// constant tree captures nothing, so each lambda names the state itself.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto stepper(lv::tree_node auto shown) {
  return lv::row(
    {
      .flex_align = {"center", "center", "center"},
      .size       = {lv::size_content, lv::size_content},
    },
    lv::button({.on_clicked = [] { --Count; }}, lv::label({.text = "-"})),
    shown,
    lv::button({.on_clicked = [] { ++Count; }}, lv::label({.text = "+"}))
  );
}
```

Checked: no tool checks this rule yet.

Guide: [State and bindings](state.md).

### no-boilerplate-4

**Show rows that change with `lv::each` over an `lv::list`, and change the list, never the widgets.**

The list builds, moves and deletes the widgets of one row at a time. A loop that builds rows by hand repeats that work for every change.

<!-- snippet: examples/lists/main.cpp tree -->
```cpp
constinit lv::computed has_tracks{
  [](lv::counter n) {
    return n.value > 0;
  },
  tracks.count(),
};

// Shown while the list is empty.
constexpr auto empty_note = lv::label({
  .hidden = lv::bind(has_tracks),
  .text   = "No tracks",
  .style  = {.text_color = tok.color.quiet},
});

// One row per track; the selected one is checked.
constexpr auto track_list = lv::group(
  {
    .flex_flow = "column",
    .size      = {100 * pct, lv::size_content},
    .style     = {.pad_gap = tok.space.gap},
  },
  lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)
);

// Groups: boxes without the theme's look, laid out as columns.
constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .size      = {320 * px, 240 * px},
    .style = {
      .bg_color = tok.color.screen,
      .bg_opa   = tok.opacity.opaque,
      .pad_all  = tok.space.edge,
      .pad_gap  = tok.space.gap,
    },
  },
  empty_note, track_list
);
```

Checked: no tool checks this rule yet.

Guide: [Lists](lists.md).

### no-boilerplate-5

**Write a set that is fixed when the program is written with `lv::for_each`: a part for each enumerator, captioned member or constant record.**

The compiler writes the repeated widgets, so a new enumerator or member gets its widget without a new line in the tree.

<!-- snippet: examples/repeat/main.cpp tiles -->
```cpp
enum class page { none, print, scan, copy, usb };
constinit lv::state<page> shown{page::none};

// The label of a tile is a member of `labels` with the name of the enumerator.
struct labels {
  static constexpr char print[] = "Print";
  static constexpr char scan[] = "Scan";
  static constexpr char copy[] = "Copy";
  static constexpr char usb[] = "USB";
};

constexpr auto tile = []<page Page> {
  return lv::button({.size = {80 * px, 60 * px}, .on_clicked = [] { shown = Page; }})[
      lv::label({.text = lv::like<Page, labels>})];
};

constexpr auto home = lv::row({.size = {100 * pct, lv::size_content}})[
    lv::for_each<page, lv::but<page::none>>(tile)];
```

Checked: no tool checks this rule yet.

Guide: [Repetition](repetition.md).

### no-boilerplate-6

**Give a dropdown or a roller its options as a list, `{"Cut", "Copy"}`, or as `lv::options_of<E>`, never as one text with line breaks.**

The compiler joins the list into LVGL's text in read-only memory, and `lv::options_of<E>` keeps the options and a state of `E` in step.

<!-- snippet: examples/options/main.cpp but -->
```cpp
// The sheets of this printer's tray: A5 is left out, so the list has three options.
constexpr auto tray = lv::options_of<sheet, lv::but<sheet::a5>>;
```

Checked: no tool checks this rule yet.

Guide: [Option lists](options.md).

### no-boilerplate-7

**Give all widgets of a kind their look in a theme, not in a style on each node.**

A theme is written once, its styles are constants in read-only memory, and the tree keeps only what differs.

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

Checked: no tool checks this rule yet.

Guide: [Themes](themes.md).

### no-boilerplate-8

**Start the program with `lv::application`, take the screen from an `lv::display`, and end `main` with `return app.run();`.**

The two objects start and stop LVGL, and `run()` is the loop. A program has no start-up code, no tick and no loop of its own.

<!-- snippet: examples/window/main.cpp main -->
```cpp
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{app, "lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display, window};       // the window's mouse feeds it

  auto root = lv::mount<app::ui>(display.screen());
  if (!root) return 2;
  app::start(root);                         // the application's task, on the mounted tree
  return app.run();                         // until the window is closed
}
```

Checked: no tool checks this rule yet.

Guide: Application and loop (not written yet).

### no-boilerplate-9

**Switch the pages of a tree with `lv::shown(state)`, and one of several dialogs at a root with an `lv::one_of` slot; never write a pager.**

The library builds the page or the dialog that is current and ends the one before, in memory of the tree's own, and allocates nothing for the switch.

<!-- snippet: examples/pages/main.cpp tree -->
```cpp
constexpr auto ui = lv::row({.size = {320 * px, 240 * px}})[
    lv::column({.size = {80 * px, 240 * px}})[
        lv::button({.on_clicked = [] { current = page::home; }})[lv::label({.text = "Home"})],
        lv::button({.on_clicked = [] { current = page::settings; }})[lv::label({.text = "Settings"})],
        lv::button({.on_clicked = [] { current = page::about; }})[lv::label({.text = "About"})]],
    lv::column({.size = {240 * px, 240 * px}})[
        lv::group({.size = {240 * px, 30 * px}})[lv::when(offline)[offline_banner]],
        lv::group({.size = {240 * px, 200 * px}})[
            lv::shown(current, {.enter = slide_in, .leave = slide_out})[
                lv::is<page::home>(home, {.on_shown = &greet, .on_front = &count_visit}),
                lv::is<page::settings>(settings),
                lv::is<page::about>(about)]]]];
```

Checked: no tool checks this rule yet.

Guide: [Where a tree lives](mounting.md).

## Fast at run time

Let the compiler do the work once, so that the device does less and allocates less while the program runs.

### fast-run-1

**Keep states, lists and mounted trees in `constinit` globals or in variables: lvgl26 takes no heap for them.**

Their sizes are known when the program is compiled. The heap then holds only the widgets and what each needs, and a screen that does not change allocates nothing.

<!-- snippet: examples/lists/main.cpp model -->
```cpp
struct track_id {
  int value;
};

struct track {
  track_id id;  // names the row
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 8> tracks;               // room for eight rows
constinit lv::state current{track_id{0}};          // the selected row; no track has the id 0
```

Checked: no tool checks this rule yet.

Measured on 2026-11-18, see [memory-region.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/memory-region.md). In the measured program only LVGL's heap was used: no operator new and no malloc. A screen that did not change allocated nothing.

Guide: Code that runs fast (not written yet).

### fast-run-2

**Change one row with `insert`, `erase`, `move` or `update`; never clear and fill a list again to change it.**

Only the widgets of that row change, and the other rows keep theirs, with their animations, their tasks and the focus.

<!-- snippet: examples/lists/main.cpp move -->
```cpp
  play(2);
  tracks.move(3, 1);      // Harbour moves up, with its widgets
  current = track_id{2};  // selects Northern Lights
```

Checked: no tool checks this rule yet.

Measured on 2026-11-17, see [lists.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/lists.md). A job added to the printer's queue of six: the port's own loop built all six cards again, and `lv::each` built none of them.

Guide: [Lists](lists.md).

### fast-run-3

**Show a text field of a row through `lv::bind`; do not copy it into a buffer of your own.**

A list keeps each row at one address, so a label shows the row's text where it is. A copy costs heap in every row.

<!-- snippet: examples/lists/main.cpp row -->
```cpp
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
```

Checked: no tool checks this rule yet.

Measured on 2026-11-17, see [lists.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/lists.md). A row of two labels took 960 bytes of LVGL's heap through `lv::each`, and 1094 bytes as a tree of its own with copied texts.

Guide: [Lists](lists.md).

### fast-run-4

**Start work that belongs to a part of the screen with `lv::spawn(task, owner)`, so that it ends when the owner is deleted.**

A task that outlives its widgets keeps its frame and its timers, and goes on working for a screen that nobody sees.

No example yet: the Tasks guide will add one.

Checked: no tool checks this rule yet.

Measured on 2026-11-18, see [memory-region.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/memory-region.md). A task's frame is one block of LVGL's heap. Each sleep adds a timer of 36 bytes, each watched widget 16 bytes and a delete handler of about 24.

Guide: Tasks (not written yet).

### fast-run-5

**Change a mounted widget with `set({...})` written in place, and give it a named style or a look rather than `lv::style({...})` built in the call.**

One `set` takes about 3.5 KB of stack on a Cortex-M4, and a style built inside the call adds about 2.6 KB more.

No example yet: the `set` of the mounting example sits next to test code. A clean one comes with the Views and ids guide.

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [memory.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/memory.md). One `set({.value = v})` took 3,504 bytes of stack on a Cortex-M4 at -Os; with `lv::style({...})` inside it 6,120 bytes; with a look 3,504 bytes. The nightly probe `set_stack` measures it again.

Wins over a style written inline where it is used: on a Cortex-M4 the 2.6 KB of stack decide.

Guide: Code that runs fast (not written yet).

### fast-run-6

**Let the compiler make the fonts and pictures known at build time; decode on the device only photos and sizes chosen at run time.**

A font or a picture made by the compiler is constant data in flash, drawn with no decoder and no heap.

<!-- snippet: examples/fonts/main.cpp typeface -->
```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto type = lv::typeface({
    {"Barlow", 500, barlow_ttf},
    {"Barlow Condensed", 600, condensed_ttf, "0123456789:"},  // only what a clock shows
});

constexpr auto label = type(tok.text.label);
constexpr auto title = type(tok.text.title);
constexpr auto numeral = type(tok.text.numeral);
```

Checked: no tool checks this rule yet.

Measured on 2026-11-18, see [memory-region.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/memory-region.md). Fonts and pictures the compiler makes were constants in flash. A font drawn on the device kept 1.3 KB of heap for its cache. A PNG decoded on the device took 164 KB for a moment.

Wins over `fast-build-2` where the two meet. The device pays for a decoder each time the program runs; the build pays for a drawn font only when its file changes.

Guide: [Fonts](fonts.md).

### fast-run-7

**Turn off what the product does not use: `LVGL26_NO_DEFAULT_THEME` when your theme covers every widget, and the options of the decoders.**

LVGL's default theme takes heap for the first display, under a theme that replaces it anyway.

No example yet: it is one CMake line, `set(LVGL26_NO_DEFAULT_THEME ON)`, which no example sets.

Checked: no tool checks this rule yet.

Measured on 2026-11-16, see [memory.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/memory.md). LVGL built its default theme for the first display: 2532 bytes of styles on its heap.

Guide: [Themes](themes.md).

### fast-run-8

**When nobody sees the screen, say so with `display.hide()`: a display that is not visible draws nothing until it is shown again.**

LVGL keeps drawing into a frame that nobody sees, and only the program knows that the window is hidden or the panel is off.

No example yet: the window platform hides its display; the guide Application and loop will show it.

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [visibility.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/visibility.md). 24 animated arcs drawn into a frame nobody saw took 438 M instructions a second. With nothing marked for redraw, as on a hidden display, they took 15 M. The nightly probe `visibility_invalidation_off` measures it again.

Guide: Application and loop (not written yet).

### fast-run-9

**Hide a widget that nobody should see with `.hidden`; do not cover it or make it transparent.**

LVGL skips a hidden widget when it draws. A covered widget, or one at opacity zero, is still drawn every frame.

<!-- snippet: examples/state/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::dropdown({.options = lv::options_of<drink>, .selected = lv::bind(chosen), .width = 140 * px}),
  stepper<cups>(cups_text),
  lv::slider({.range = {1, 5}, .value = lv::bind(bold), .width = 200 * px}),  // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
```

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [visibility.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/visibility.md). 24 animated arcs took 438 M instructions a second visible, and 0.6 to 0.9 M hidden. Covered by an opaque sibling they took 42.5 M; at opacity zero, 74.9 M. The nightly probe `visibility_hidden` measures it again.

Guide: Code that runs fast (not written yet).

### fast-run-10

**Make a property follow a state through `lv::bind`; never poll a value in a timer or a loop to set the property.**

A binding works only when the value changes, and sets only the widgets that show it. A poll works on every tick, changed or not.

<!-- snippet: examples/state/main.cpp smallest -->
```cpp
constinit lv::state cups{lv::counter{2}};  // a value that widgets follow

constexpr auto cups_text = lv::label({.text = lv::bind(cups, "{} cups")});  // shows it, and follows it
```

Checked: no tool checks this rule yet.

Measured: not measured yet.

Guide: [State and bindings](state.md).

### fast-run-11

**Change only what changed: set or bind the one property, and never mount a tree again to show a new value.**

LVGL redraws the area of the widget whose property changed. A tree mounted again is built, laid out and drawn whole.

<!-- snippet: examples/state/main.cpp change -->
```cpp
  cups = lv::counter{4};  // the count, the total, the warning and the grinder follow
```

Checked: no tool checks this rule yet.

Measured: not measured yet.

Guide: Code that runs fast (not written yet).

### fast-run-12

**Animate the smallest widget that shows the motion, and let the animation end when the motion is over.**

Every frame of an animation redraws the area of the widget it moves, for as long as it runs.

No example yet: the Animation guide will add one.

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [visibility.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/visibility.md). 24 arcs of 70 by 70 pixels, animated without end on a 480 by 320 display, took 438 M instructions a second. The nightly probe `visibility_visible` measures it again.

Guide: Animation (not written yet).

## Fast to compile

Pay the compiler only for what a file uses. The sizes below are rough and relative; they are instructions of GCC 16.2, measured on one machine.

### fast-build-1

**Include `<lv/lv.hpp>` and the header of each feature the file uses; include `<lv/all.hpp>` only in a first program.**

The core costs a file about a third less than all headers together, and an opt-in header costs only the files that use its feature.

<!-- snippet: examples/lists/main.cpp includes -->
```cpp
#include <lv/list.hpp>
#include <lv/lv.hpp>
```

Checked by the build (`cmake/check_includes.cmake`), in every target that links lvgl26 through its CMake, unless LVGL26_CHECK_INCLUDES is off. The test `include_check_build` pins the message.

```text
parts.hpp:8: lv::list needs #include <lv/list.hpp>
```

Measured on 2026-11-20, see [header-structure.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/header-structure.md). A file that includes only the core: 9.01 G instructions, against 13.00 G for all headers before the split. On top of the core, look.hpp adds 3.28 G, font.hpp 1.97 G and list.hpp 0.17 G. The nightly probe `include_lv` measures it again.

Wins over the ease of one header for everything, which is worth it in a first program only.

Guide: [Which header](which-header.md).

### fast-build-2

**Name drawn fonts in few files, and give the other files their text through the display's theme.**

Every file that names a font draws it again, which costs about as much compile work as a whole screen.

<!-- snippet: examples/signal_type/main.cpp main -->
```cpp
int main() {
  lv::application app;
  lv::display display{app, frame, lv::signal::dark_with_type};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "signal_type.png") ? 0 : 2;
}
```

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [compile-diagnosis.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/compile-diagnosis.md). A file with one mounted label: 54 G instructions. A drawn font adds about 45 G to a file. Five fonts were 234 G of the 341 G of the printer's Print page. The nightly probe `one_label` measures it again.

Guide: Keeping builds fast (not written yet).

### fast-build-4

**Let the compiler decode pictures up to the limit in the settings, and decode larger ones on the device with `lv::encoded_image`.**

The compiler's work grows with the pixels, so a large picture makes every build of its file slow.

<!-- snippet: examples/images/main.cpp files -->
```cpp
constexpr unsigned char sky_png[] = {
#embed "sky.png"
};
constexpr unsigned char sun_png[] = {
#embed "sun.png"
};

constexpr auto sky = lv::image_file<sky_png>;  // every pixel opaque: stored as the display's own pixels
constexpr auto sun = lv::image_file<sun_png>;  // see-through around the sun: stored with an alpha plane
```

Checked by the compiler. The test `image_too_large` pins the message.

```text
error: constexpr message: lv: this picture is 200 x 100 pixels, and the compiler decodes at most 16384 (a large picture makes the build slow). To allow it, raise limits.compile_time_image in the settings.
```

Measured: not measured yet.

Guide: [Images](images.md).

### fast-build-5

**Bind, name and style nodes freely: after its first use in a file, a binding, an id or a property costs little to compile.**

The compiler pays for a widget kind once in each file; what one node adds on top of it is small.

<!-- snippet: examples/state/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::dropdown({.options = lv::options_of<drink>, .selected = lv::bind(chosen), .width = 140 * px}),
  stepper<cups>(cups_text),
  lv::slider({.range = {1, 5}, .value = lv::bind(bold), .width = 200 * px}),  // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
```

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [compile-diagnosis.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/compile-diagnosis.md). Ten labels: 55.8 G instructions. Each bound to a state: 56.0 G. Each with an id: 55.8 G. Each with three more properties: 59.1 G. The nightly probe `large_tree` measures it again.

Guide: Keeping builds fast (not written yet).

## Checked at compile time

Write the code so that a mistake stops the build, with a message that says how to fix it.

### checked-1

**Give every value a type that says what it is: a quantity, an `lv::counter`, or a struct of one member.**

A length is then no speed, a speed converts by its unit, and the compiler refuses a state of a plain integer.

<!-- snippet: examples/state/main.cpp typed -->
```cpp
enum class drink { espresso, lungo, americano };  // a choice: an enum
struct strength {                                 // a value with a name: a struct of one member
  int value;
};

constinit lv::state chosen{drink::lungo};
constinit lv::state bold{strength{3}};
constinit lv::state beans{60 * g};  // a measure: a quantity with its unit
```

Checked by the compiler. The test `state_plain_int` pins the message.

```text
error: static assertion failed: lv: a state of a plain integer does not say what the number is. If it is a measure, give it a unit with mp-units: `lv::state speed{0 * km / h};`. If it counts things, use lv::counter: `lv::state clicks{lv::counter{}};`. For anything else, name it: `struct tab_index { int value; };`. To allow plain integers, set rules.raw_integer_state in the settings.
```

Guide: [State and bindings](state.md).

### checked-2

**Write an LVGL enumerator as its name in quotes, `.align = "center"`; a misspelled name stops the build and lists the right ones.**

The names are read from LVGL's headers, so they are complete, and the same on both LVGL versions.

<!-- snippet: examples/lists/main.cpp row -->
```cpp
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
```

Checked by the compiler. The test `enumerator_unknown_name` pins the message.

```text
error: constexpr message: lv: align has no value 'centre'. Its values: default, top_left, top_mid, top_right, bottom_left, bottom_mid, bottom_right, left_mid, right_mid, center, out_top_left, out_top_mid, out_top_right, out_bottom_left, out_bottom_mid, out_bottom_right, out_left_top, out_left_mid, out_left_bottom, out_right_top, out_right_mid, out_right_bottom.
```

Guide: The tree and its widgets (not written yet).

### checked-3

**Give a style design tokens, and mark a value that no token should name with `lv::raw(...)`.**

The design then lives in one token file, and a value typed straight into a style stops the build.

<!-- snippet: examples/lists/main.cpp row -->
```cpp
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
```

Checked by the compiler. The test `style_needs_token` pins the message.

```text
error: static assertion failed: lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a length (120 * lv::px, 50 * lv::pct or lv::size_content). You gave lv::length. A bare value is no token: write lv::raw(...) around it, or take it from the token file.
```

Wins over the shorter raw value: one source for the design is worth the name of a token.

Guide: Design tokens (not written yet).

### checked-4

**Name one settings file for the program, and forbid in it what the product may not use: allocation, threads, files, floating point.**

A part of lvgl26 that needs a forbidden feature is then a compile error that names the key, and a restriction changes no generated code.

<!-- snippet: examples/settings/presets/microcontroller.json -->
```json
{
  "$schema": "https://raw.githubusercontent.com/filipsajdak/lvgl26/main/docs/reference/lvgl26-settings.schema.json",
  "allow": {"allocation": false, "threads": false, "files": false, "floating_point": false},
  "limits": {"compile_time_image": {"value": 4096, "unit": "px"}}
}
```

Checked by the compiler. The test `restrict_from_file` pins the message.

```text
error: static assertion failed: lv: lv::ui_scheduler needs threads, and this project does not allow them (allow.threads in settings/restricted.json).
```

Guide: [Project settings](settings.md).

### checked-5

**Handle the `std::expected` that `lv::mount`, `insert` and `assign` return: a full list or a heap without room is a value, not a crash.**

On a small device memory runs out; the result says so, nothing is left half built, and the program decides what to show.

<!-- snippet: examples/lists/main.cpp fill -->
```cpp
  if (!tracks.assign(playlist)) return 2;  // builds a row for each track: full, or no memory
```

Checked: no tool checks this rule yet.

Guide: [Where a tree lives](mounting.md).

### checked-6

**Read the first error and its `note:` lines before anything else, and do what the message says.**

An lvgl26 message names the fix: the header, the token, the unit, the settings key or the property.

No code: the rule is about the build's output. Each guide shows the messages of its feature.

Checked: no tool checks this rule yet.

Guide: [Which header](which-header.md).

### checked-7

**Keep lvgl26's headers out of a precompiled header.**

With GCC 16.2 a precompiled header drops the `pre` and `post` checks of templates without a word: GCC bug 128909.

No code: the rule is about the build. CMake's `target_precompile_headers` must not name a header of lvgl26.

Checked: no tool checks this rule yet.

Measured on 2026-11-19, see [compile-diagnosis.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/compile-diagnosis.md). A warm precompiled header took a file with one label from 54 G to 6 G instructions: the largest build saving measured.

Wins over the largest build saving there is: a check that silently goes away is worse than a slow build.

Guide: Keeping builds fast (not written yet).

### checked-8

**Reach LVGL's C API only through `raw()`, and only for what lvgl26 has no form for; never to quiet an error.**

Code that names no C function stays checked by the compiler, and keeps working on both LVGL versions.

No example: the examples name no C function, and `tools/check_examples.py` keeps them so in this repository.

Checked: no tool checks this rule yet.

Guide: Extending lvgl26 (not written yet).

### checked-9

**Reach a widget of a mounted tree by its id, `root->get<"title">()`; never keep a pointer to it or search for it at run time.**

The compiler turns the id into the widget's place in the tree, and gives a view of its kind. A wrong id stops the build.

<!-- snippet: examples/mounting/main.cpp mount-global -->
```cpp
  if (!home_page.mount(display.screen())) return 1;
  home_page->get<"title">().set({.text = "Ready"});
```

Checked by the compiler. The test `unknown_id` pins the message.

```text
error: constexpr message: lv: the tree has no id 'titel'. Its ids: title.
```

Guide: Views and ids (not written yet).

## Expressive

Write what the screen means, in the words of the product, and let the compiler derive the rest.

### expressive-1

**Write a choice as an enum, and let the compiler derive from it: `lv::options_of<E>` for its options, `lv::match` for one arm per value.**

A new enumerator then reaches every place that lists the choices, and an `lv::match` that misses one stops the build.

<!-- snippet: examples/state/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::dropdown({.options = lv::options_of<drink>, .selected = lv::bind(chosen), .width = 140 * px}),
  stepper<cups>(cups_text),
  lv::slider({.range = {1, 5}, .value = lv::bind(bold), .width = 200 * px}),  // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
```

Checked by the compiler. The test `match_missing_case` pins the message.

```text
error: static assertion failed: lv::match does not cover: sport. Add an arm for each, or a last arm `lv::otherwise.then(...)`.
```

Guide: [Option lists](options.md).

### expressive-2

**Name a value made from states as an `lv::computed`, and bind it like a state: `.hidden = lv::bind(enough)`.**

The tree then reads as what the screen means, and a computed that nothing binds stores nothing and does no work.

<!-- snippet: examples/state/main.cpp computed -->
```cpp
using grams = decltype(beans.get());

constexpr auto least_dose = 12 * g;  // the dose of the mildest cup
constexpr auto dose_step = 2 * g;    // what one step of strength adds

// Values made from states, and from each other. They store nothing; binding one makes it follow.
constinit lv::computed dose{[](strength s) { return least_dose + s.value * dose_step; }, bold};
constinit lv::computed needed{[](lv::counter c, grams d) { return c.value * d; }, cups, dose};
constinit lv::computed enough{[](grams n, grams b) { return n <= b; }, needed, beans};
```

Checked: no tool checks this rule yet.

Guide: [State and bindings](state.md).

### expressive-3

**Write a sequence in time, such as wait for a click, pause, then animate, as one `lv::task` coroutine, not as callbacks and timers.**

The steps read in the order they happen, and the task ends with its owner.

No example yet: the Tasks guide will add one.

Checked: no tool checks this rule yet.

Guide: Tasks (not written yet).

### expressive-4

**Name the look of an element, `lv::look<"tile">`, and keep its values in the token file.**

The tree says what an element is, the token file says how it looks, and a designer can change the look without touching the code.

<!-- snippet: examples/looks/screen.hpp tree -->
```cpp
constexpr auto tile = []<action Action> {
  return lv::button(
    {.style = lv::look<"tile"> | lv::look_of<"tile", Action>},
    lv::label({.text = lv::like<Action, names>})
  );
};

constexpr auto ui = lv::group(
  {.style = lv::look<"page">},
  lv::label({.text = "Printer", .style = lv::look<"title">}),
  lv::group({.style = lv::look<"tiles">}, lv::for_each<action, lv::but<action::none>>(tile)),
  lv::label({.checked = true, .text = "Ready to print", .style = lv::look<"status">})
);
```

Checked: no tool checks this rule yet.

Wins over a little build time: `<lv/look.hpp>` adds about 3.3 G instructions to a file that uses it, less than one new widget kind.

Guide: [Looks](looks.md).

### expressive-5

**Use semantic and component tokens in screen code, such as `tok.color.selected`, not primitives such as a step of a palette.**

A semantic name says why the value is there, and survives a new palette or a dark mode.

<!-- snippet: examples/lists/main.cpp row -->
```cpp
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
```

Checked: no tool checks this rule yet.

Guide: Design tokens (not written yet).

### expressive-6

**Build a screen from named pieces: small constants, each at most about three levels deep, that the screen lists.**

A name says what a piece is, the screen shows its shape at a glance, and a piece can move to a file of its own.

No example yet: the built example comes with the reformat of the examples to children as call arguments.

Checked: no tool checks this rule yet.

Guide: The tree and its widgets (not written yet).

## Extendable

Grow the application, reuse its parts and extend lvgl26 itself, without changing what already works.

### extendable-1

**Put one screen in each file with the states only it uses, and give shared state, parts, design, platform and `main` places of their own.**

`main` only starts the application, and only the platform's file names the platform. A change then rebuilds one screen, and a reader finds each thing where the layout puts it.

No example yet: a small program laid out by screen comes with the guide An application that grows.

Checked: no tool checks this rule yet. Not built yet: a layout check, switched on in CMake like the include check, for a file with two screens, a screen that includes another screen's header, a font drawn outside `design/` and a platform name outside `platform/`.

Wins over the total work of a build. Each file pays again for its first widget, about 35 G instructions, but a change rebuilds one file.

Guide: An application that grows (not written yet).

### extendable-2

**Make a reusable piece of screen a function that returns a tree, and constrain each parameter with a public concept: `lv::dependency`, `lv::tree_handler`, `lv::tree_node`.**

A wrong argument is then refused at the call, with the name of the concept, and not deep inside the part.

<!-- snippet: examples/state/main.cpp part -->
```cpp
// A count between two buttons. The part names its state as a template argument: a handler in a
// constant tree captures nothing, so each lambda names the state itself.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto stepper(lv::tree_node auto shown) {
  return lv::row(
    {
      .flex_align = {"center", "center", "center"},
      .size       = {lv::size_content, lv::size_content},
    },
    lv::button({.on_clicked = [] { --Count; }}, lv::label({.text = "-"})),
    shown,
    lv::button({.on_clicked = [] { ++Count; }}, lv::label({.text = "+"}))
  );
}
```

Checked by the compiler. The test `part_dependency_requires_clause` pins the message.

```text
error: no matching function for call to 'shown<plain>()'
candidate 1: 'template<auto& Count>  requires  dependency<typename std::remove_cvref<decltype(Count)>::type> constexpr auto shown()'
```

Guide: [Concepts for parts](concepts.md).

### extendable-3

**Give a part its state as a template argument, `stepper<cups>(...)`, so that its handlers can name the state.**

A handler in a constant tree is a plain function pointer and captures nothing; a template argument is a constant that it can name.

<!-- snippet: examples/state/main.cpp part -->
```cpp
// A count between two buttons. The part names its state as a template argument: a handler in a
// constant tree captures nothing, so each lambda names the state itself.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto stepper(lv::tree_node auto shown) {
  return lv::row(
    {
      .flex_align = {"center", "center", "center"},
      .size       = {lv::size_content, lv::size_content},
    },
    lv::button({.on_clicked = [] { --Count; }}, lv::label({.text = "-"})),
    shown,
    lv::button({.on_clicked = [] { ++Count; }}, lv::label({.text = "+"}))
  );
}
```

Checked by the compiler. The test `part_handler_capturing_lambda` pins the message.

```text
error: no matching function for call to 'tap_button(make(int)::<lambda()>)'
concept tree_handler = detail::without_captures__name_the_state_as_a_template_argument<F>;
```

Guide: [Concepts for parts](concepts.md).

### extendable-4

**Keep the design in a token file named once for the project, and let a library of parts take the design as a template parameter.**

A library of parts then serves several products, each with its own tokens, and no part names a file.

<!-- snippet: examples/looks/screen.hpp design -->
```cpp
template <>
inline constexpr lv::design_of<decltype(looks::tok)> lv::design<lv::override>{
    .tokens = looks::tok, .type = lv::typeface({{"Barlow", 500, looks::barlow}})};
```

Checked: no tool checks this rule yet.

Guide: [Looks](looks.md).

### extendable-5

**Connect a platform as an object that feeds `lv::display` and `lv::pointer` and joins the loop with `lv::in_loop`; the application names no platform.**

The same application then runs in a window, in a browser and on a board, and each platform is one file.

<!-- snippet: examples/window/main.cpp main -->
```cpp
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{app, "lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display, window};       // the window's mouse feeds it

  auto root = lv::mount<app::ui>(display.screen());
  if (!root) return 2;
  app::start(root);                         // the application's task, on the mounted tree
  return app.run();                         // until the window is closed
}
```

Checked: no tool checks this rule yet.

Guide: [A pointer](input.md).

### extendable-6

**Reach a widget that has no short name with `lv::widget<"spinbox">`: its properties and events come from LVGL's headers, as for every widget.**

Nothing is written by hand for one widget, so a widget of a new LVGL version works once that version builds.

No example yet: the ports of LVGL's spin box examples use it, without a marked part to show.

Checked: no tool checks this rule yet.

Guide: Extending lvgl26 (not written yet).
