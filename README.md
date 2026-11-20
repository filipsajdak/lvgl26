# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

One UI framework, from a low-cost microcontroller up. lvgl26 and LVGL are both MIT: no royalty per device and no licence per developer seat. It is designed for AI agents as much as for people, to speed up migration, development, testing and verification.

Documentation: start with the [quick start](docs/quick-start.md). The site is at <https://filipsajdak.github.io/lvgl26/>.

Status: early. Typed widget views, the declarative tree, styles, design tokens, themes, fonts, images, state, derived state, lists, concepts for reusable parts and coroutines exist. The first agent tools exist: a tree dump, screenshots and an MCP server (`lv/mcp.hpp`, `examples/agent`).

```cpp
#include <lv/all.hpp>
using namespace lv::units;

void reset();

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
    lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];

auto root = lv::mount<ui>(display.screen());      // built in place; owns the widgets
root->get<"speedo">().set({.value = 0});          // a typed view of the arc
```

How it works:

- **Properties are generated.** `lv::props<"arc">` is a struct that C++26 reflection builds from LVGL's
  own C API: one member per `lv_arc_set_*` and `lv_obj_set_*` function and one per event (`on_clicked`).
  Nothing is written by hand and nothing is generated offline.
- **Properties are typed.** Sizes take `120 * px`, `50 * pct` or `lv::size_content`; angles `90 * deg`; times
  `300 * ms`; opacity `50 * pct`. Integers reject `bool`, floating point and values that do not fit.
- **An angle is in the unit LVGL means.** Most of LVGL takes whole degrees (an arc, a scale): that is an
  `lv::angle`. An image's rotation and a style's `transform_rotation` and `transform_skew_x/y` take tenths
  of a degree: that is `lv::decidegrees`. Write `90 * deg` for both; `22.5 * deg` only compiles where
  tenths are taken. LVGL says which is which only in its doc comments.
  lvgl26 reads them from the embedded headers at compile time (`include/lv/detail/angle_docs.hpp`).
  A test checks the result against the pixels LVGL draws.
  A setter that takes its value unsigned, such as a spinner's `.arc_sweep = 270 * deg`, gets an `lv::nonnegative<lv::angle>`.
  A negative constant there is a compile error, and a negative value at run time breaks a contract.
- **An LVGL enumerator is a name in quotes.** `.align = "center"`, `.flex_flow = "row_wrap"`,
  `.flex_align = {"center", "center", "start"}`.
  The name is the enumerator without what all of its enum share, in lower case, so `LV_ALIGN_TOP_LEFT` is `"top_left"`.
  An internal enumerator that LVGL marks with an underscore keeps its whole name and does not shorten the others.
  A name may stand beside values of other kinds: a roller takes `.options = {{"Mon", "Tue"}, "infinite"}`.
  A wrong name is a compile error that lists the names.
- **Mistakes are compile errors.** A wrong value says what the property takes and what you gave.
  A wrong id lists the ids of the tree.
- **The tree costs nothing at run time.** `lv::mount<ui>` reads it at compile time; an optimized build
  contains only the LVGL calls for the properties that are set.
- **Views never delete.** `lv::view<"arc">` is one pointer. `lv::mount` and `lv::owned<T>` own widgets;
  `lv::weak<T>` empties itself when LVGL deletes the widget.
- **A mounted tree stays where it is built.** `lv::mounted` owns the widgets and holds what the tree's
  nodes keep at run time, sized by the compiler: lvgl26 allocates nothing for it. It is never moved, and
  carries its own failure: `if (!root) return root.error();`. It lives in a variable, in a `constinit`
  global with `page.mount(parent)` and `page.unmount()`, for a window that opens and closes in the
  same memory. See [Where a tree lives](docs/mounting.md).
- **Pages that follow a state** (`#include <lv/shown.hpp>`). `lv::shown(current)[lv::is<page::home>(home), lv::otherwise()]`
  shows the tree of the value of a state of an enum or a bool, and `lv::when(offline)[banner]` a tree while a flag is
  true. Every value has one arm, checked by the compiler. `lv::shown(current, {.enter = slide_in, .leave = slide_out})`
  slides pages with `lv::transition`: the new page is built before the old one leaves. A page of another file is
  `extern const lv::page home_page;`, defined there as `lv::page_of<ui>`, with no size to write. A switch allocates
  nothing of lvgl26's; `lv::settle()` switches at once. See `examples/pages`.
- **One slot for one of several trees** (`#include <lv/one_of.hpp>`). `lv::one_of<saving, failed>` holds one tree at a time in the
  memory of the largest, as `std::variant` does. `slot.mount<failed>(parent)` ends the tree that is there
  first, and `slot.visit(...)` calls the function whose parameter names the tree that is mounted:
  `lv::held<failed>`, a handle checked at each use, `auto` for the others, `lv::not_mounted` for the
  empty slot. The result is the arms' common type, or the `R` of `slot.visit<R>(...)`.
- **A mouse or a touch panel without C** (`#include <lv/pointer.hpp>`). `lv::pointer mouse{display}` is fed
  by the platform: `mouse.press(at)`, `mouse.move(to)`, `mouse.release()`. A queue inside the object keeps a
  press shorter than LVGL's read period, and LVGL reads it at the next step. `lv::pointer panel{display, &read}`
  is asked at each read instead. The window examples feed it from SDL. See [A pointer](docs/input.md).
- **One settings file for the whole program.** `lvgl26_settings(app lvgl26.json)` names it in CMake, and
  every file of the program reads it. It forbids what the program may not use (`"allow": {"threads": false}`:
  then `lv::ui_scheduler` is a compile error that names the key), relaxes the library's rules, and bounds the
  compiler (`"limits": {"compile_time_image": {"value": 4096, "unit": "px"}}`). A misspelled key is a compile
  error too. See [Project settings](docs/settings.md).
- **The window joins the application's loop.** `main` ends with `return app.run();` on a device and in a
  window alike: a platform object joins the loop with `lv::in_loop` and is polled before each step, and a
  closed window ends `run()` unless the program takes the request. A hidden window keeps its trees and
  draws nothing. A host that owns the loop, such as a browser page, calls `step()`.
- **A small core header.** `#include <lv/lv.hpp>` is what nearly every program writes. A theme, a look,
  fonts, pictures, lists, repetition, documents, slots and the pointer each have a header of their own,
  and `#include <lv/all.hpp>` gives everything. [Which header](docs/which-header.md) lists the names of
  each. A build through lvgl26's CMake stops at a missing header with
  `main.cpp:12: lv::list needs #include <lv/list.hpp>`, and clangd offers the include as a quick fix.
- **Coming from another framework.** Guides for [Qt Quick](docs/from-qt.md), [Slint](docs/from-slint.md) and [TouchGFX](docs/from-touchgfx.md) map what you know, and the [examples](examples/README.md) hold the ports.

State is typed and lives in ordinary globals. Widgets follow it. Measures are
[mp-units](https://github.com/mpusz/mp-units) quantities:

```cpp
#include <mp-units/systems/si.h>
using namespace mp_units::si::unit_symbols;

constinit lv::state speed{0 * km / h};
constinit lv::state clicks{lv::counter{}};

constexpr auto ui = lv::column[
    lv::arc({.range = {0, 240}, .value = lv::bind(speed)}),   // follows, and writes back
    lv::label({.text = lv::bind(speed)}),                     // shows "0 km/h"
    lv::label({.text = lv::bind(clicks, "{} clicks")})];

speed = 120 * km / h;                                  // the widgets update
speed = 2 * km / min;                                  // converted: nothing is lost
speed = 33 * m / s;                                    // error: 118.8 km/h does not fit an int
speed = 3 * m;                                         // error: not a speed
++clicks;
```

What a state can hold:

- **A measure:** an mp-units quantity with an integer number. lvgl26 has no unit system of its own and
  does not include mp-units; it accepts any type of that shape. `set(LVGL26_UNITS ON)` in CMake adds mp-units for you.
  It builds freestanding, and a quantity compiles to the same code as an `int`.
- **A count:** `lv::counter`.
- **Anything else that is a number:** a struct with one integer, bool or enum member, such as
  `struct tab_index { int value; };`.
- A bool, an enum, a color or a pointer (`const char*` for text).

`lv::state<int>` is rejected, because a plain integer does not say what it is; the error shows these
choices. `rules.raw_integer_state` in the [settings](docs/settings.md) allows it.

A state keeps its own list of watchers, so following it costs no LVGL subject and no observer.
`speed.raw()` gives an `lv_subject_t`, made on first use, and C code can share it: a value C code writes reaches the widgets, and a value set from C++ reaches C observers.
The watchers of lvgl26 are told first, then the C observers, whatever the order in which they were added.
A property follows through a watcher of the state; the properties that LVGL animates once drawn (a bar, a slider, a roller) and `checked` keep LVGL's own bind function, and with it a subject and an observer: LVGL animates them after the first draw, and the public API does not say when that is.
A roller and a dropdown follow a state with `.selected = lv::bind(day)`, and write the user's choice back.

A value made from states is an `lv::computed`, and an action that follows them an `lv::effect`.
The dependencies are part of the type, so both can be constant-initialized globals:

```cpp
constinit lv::state limit{50 * km / h};
constinit lv::computed over_limit{[](auto s, auto l) { return s > l; }, speed, limit};
constinit lv::effect log{[](auto s) { std::printf("%d\n", s.numerical_value_in(km / h)); }, speed};

lv::label({.hidden = lv::bind(over_limit), .text = "Within the limit"})   // bound like a state

log.start();                                           // runs now, and after every change
```

`over_limit.get()` calls the function with the current values; nothing is stored.
Binding a computed makes it watch each dependency, and keeps its last result to tell only a change; `raw()` creates its `lv_subject_t` on first use.
The result follows the rules of a state, so a function that returns a plain `int` is rejected.
See `include/lv/computed.hpp` for how changes travel and who owns what.

A list shows a row template for each row of an `lv::list`, and a change touches the widgets of its
own row only:

```cpp
struct track_id { int value; };
struct track { track_id id; const char* title; lv::counter plays; };

constinit lv::list<track, 20> tracks;                   // room for 20 rows, no heap of its own
constinit lv::state current{track_id{0}};

constexpr auto ui = lv::column[
    lv::each(tracks, &track::id, {.selected = lv::bind(current)})[
        lv::row[lv::label({.text = lv::bind(&track::title)}),
                lv::label({.text = lv::bind(&track::plays, "{} plays")})]]];

auto added = tracks.push_back({{1}, "Intro"});          // builds one row, or says the list is full
added = tracks.insert(0, {{2}, "Outro"});               // one more; the first keeps its widgets
tracks.update(1, [](track& t) { ++t.plays; });          // sets that row's labels again
```

- The key names a row: `assign()` keeps the widgets of every key it finds again.
- A text field is shown from the row itself, without a copy.
- The selected row is `checked`, so `.style_checked` styles it. [docs/lists.md](docs/lists.md) has the rest.

An element names its look, and the token file says how it looks. A look is a group of the design tokens, used as a style:

```cpp
lv::button({.style = lv::look<"tile"> | lv::look_of<"tile", Page>})   // the groups "tile" and "tile-print"
lv::label({.checked = true, .style = lv::look<"status">})             // "checked" in the group is a layer
```

- The compiler matches each entry of the group to the style property of its name, checks its type, and builds a constant style in flash: no heap from lvgl26, nothing paid in a file that names no look.
- A misspelled path, an entry that fits no property and a wrong type are compile errors that list what is valid. `tools/look_groups.py` lists the groups that no look names. [docs/looks.md](docs/looks.md) has the rest.

A set that is fixed when the program is written is repeated by the compiler, not by a list at run time:

```cpp
constexpr auto tile = []<page Page> {
  return lv::button({.on_clicked = [] { shown = Page; }})[lv::label({.text = lv::like<Page, labels>})];
};
constexpr auto home = lv::row[lv::for_each<page, lv::but<page::none>>(tile)];   // one tile for each enumerator
// lv::for_each_field<job>(part): one line for each member with [[=lv::caption{"key"}]]
// lv::for_each<levels>(part): one element for each record of a constant array
```

- The widgets are children of the tree, as if written by hand: no list, no memory from lvgl26. [docs/repetition.md](docs/repetition.md) has the rest.

A dropdown and a roller take their options as a list, not as a text with line breaks. The compiler joins the list and keeps it in read-only memory:

```cpp
lv::dropdown({.options = {"100%", "75%", "50%"}})
lv::dropdown({.options = lv::options_of<quality>, .selected = lv::bind(chosen)})  // chosen holds a quality
lv::roller({.options = {{"Mon", "Tue", "Wed"}, "infinite"}})
```

- `lv::options_of<E>` makes one option for each enumerator, `lv::but<...>` leaves some out, and a state of the enum follows the choice both ways.
- An empty option or one with a line break is a compile error. [docs/options.md](docs/options.md) has the rest.

A reusable part is a function that returns a piece of tree. Its parameters are constrained by the concepts of namespace `lv`, and a captureless lambda is a handler:

```cpp
constexpr auto spin_box(const char* name, const lv::dependency auto& count, lv::tree_handler auto fewer,
                        lv::tree_handler auto more) {
  return lv::row[lv::label({.text = name}), lv::button({.on_clicked = fewer}), lv::label({.text = lv::bind(count, "{}")}),
                 lv::button({.on_clicked = more})];
}

constexpr auto ui = spin_box("Copies", copies, [] { --copies; }, [] { ++copies; });
```

- A wrong argument is refused at the call, with the name of the concept. [docs/concepts.md](docs/concepts.md) lists them all.
- A lambda with a capture is refused where the tree takes it, and the message says what to do.

Sequences of UI steps are coroutines on LVGL's loop:

```cpp
lv::task<> count_clicks(lv::view<"button"> button) {
  for (;;) {                             // ends by itself when the button is deleted
    co_await button.next().clicked;
    co_await lv::sleep(100 * ms);
    ++clicks;
  }
}

lv::task<bool> confirm(lv::view<"button"> ok, lv::view<"button"> cancel) {
  co_return co_await lv::first(
      ok.next().clicked.then([] { return true; }),
      cancel.next().clicked.then([] { return false; }),
      lv::sleep(5 * sec).then([] { return false; }));
}

lv::spawn(count_clicks(button));
lv::spawn(blink(lamp), panel);   // belongs to the panel: it ends when the panel is deleted
```

- `view.next()` names every LVGL event of a widget (`clicked`, `value_changed`, ...); the names are
  generated from LVGL's event list.
- `lv::first` waits for whichever comes first, cancels the rest, and returns what the winner's
  handler returns.
- **A deleted widget cancels the task.** Once a task has waited on a widget it is tied to it:
  deleting the widget destroys the task, running the destructors of its local objects. Wait for
  `view.next().gone` to react to the deletion instead.
- Everything runs on the UI thread. Coroutine frames come from LVGL's allocator.

A task can also wait for an animation of one property of a widget:

```cpp
lv::task<> open(lv::view<"arc"> arc) {
  co_await lv::animate(arc, {.value = {0, 100}}, 300 * ms);
  bool finished = co_await lv::animate(arc, {.width = {80 * px, 120 * px}}, 200 * ms);
}
```

- `lv::animatable<"arc">` lists the properties that take one number (a whole number, a length, an
  angle, an opacity), generated from the same setters as `lv::props<"arc">`. Both ends have the
  property's type: `.width = {0, 100}` is a compile error, and so is `{10 * px, 50 * pct}`.
- A fourth argument says how the value moves: a name from `lv::ease` (LVGL's own paths and named
  curves, such as `lv::ease::in_quad`, `out_back`, `bounce`), a `cubicBezier` design token
  (`tok.motion.fall`), or an `lv::bezier` as in CSS. Without it the value moves at one speed.
- The result is `false` when something stopped the animation: a newer animation of the same
  property took over, or `lv_anim_delete()` removed it.
- Cancelling the task, or losing in `lv::first`, deletes the animation.
- [examples/bounce/scene.hpp](examples/bounce/scene.hpp) is a whole scene in motion: a ball that falls, bounces lower each time and squashes, with a shadow under it. `tests/example_bounce.cpp` steps its clock and pins the motion, and its pictures at fixed times.

A program starts LVGL with two objects and calls no C function:

```cpp
constinit lv::frame<320, 240> frame;   // the display's memory

int main() {
  lv::application app;                 // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame};     // no panel: the picture stays in the frame

  auto root = lv::mount<ui>(display.screen());
  return app.run();                    // timers, animations and input, until app.quit()
}
```

- **Time.** `lv::application app{lv::host}` takes the operating system's clock (`#include <lv/host.hpp>`).
  A device passes its own, `{.clock = &since_start}`, or tells the time from a timer interrupt
  with `app.advance(5 * ms)`.
- **A panel.** `lv::display display{app, frame, &changed}` calls `void changed(lv::area)` with the
  part of the frame that is new. An `lv::band<320, 240, 24>` holds 24 lines instead of the whole
  screen; its panel function gets each piece: `void send(lv::area, std::span<const std::byte>)`.
- **The memory is right by its type.** A frame or a band has the size and the alignment LVGL
  asks for. LVGL ignores a buffer that is not aligned, and the screen stays empty.
- `app.heap()` says how much of LVGL's heap is in use.

Another thread never calls LVGL. It posts work to the UI thread through `lv::ui_scheduler`
(`#include <lv/ui_scheduler.hpp>`, hosted only):

```cpp
lv::ui_scheduler ui;                             // on the UI thread

ui.post([reading] { temperature = reading; });   // from any thread
co_await ui.schedule();                          // in a task: go on in a later turn of the loop
```

`post()` only locks a mutex and queues the work; the UI thread runs the queue from an LVGL timer,
in the order of posting. It needs no LVGL build option (`LV_USE_OS` may stay off).

Choosing by value is checked at compile time, in the shape of the pattern matching proposed for
C++29 (P2688):

```cpp
enum class mode { off, eco, sport };

const char* name = lv::match(m)(
    lv::is<mode::off>.then([] { return "Off"; }),
    lv::is<mode::eco>.then([] { return "Eco"; }),
    lv::is<mode::sport>.then([] { return "Sport"; }));
// leave one out: error: lv::match does not cover: sport. Add an arm for each, or ...

int level = lv::match(n)(lv::is<1, 2>.then([] { return 0; }), lv::otherwise.then([] { return 1; }));
```

For an enum, reflection checks that every enumerator has exactly one arm. A repeated value, an
`lv::otherwise` that can never match, and a missing `lv::otherwise` on a type whose values cannot be
listed are compile errors too.

Design tokens come from a W3C Design Tokens (DTCG) JSON file, read at compile time:

```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

lv::box({.style = {.bg_color = tok.color.surface, .radius = tok.radius.card}})
```

Each token group becomes a struct and each token a typed `lv::token<T>`.
The types are colors, dimensions (spacing, radius, sizes), durations, numbers (opacity), font family and weight, and easing curves.
The composite types are shadow, border, typography, transition and gradient.
Aliases are resolved. A misspelled
token, a broken alias or an unreadable value is a compile error.

**Styles use tokens.** A design value in a style (color, length, opacity, time, number) must be a token:

```cpp
.style = {.radius = tok.card.radius}                       // ok
.style = lv::style({...}) | tok.card.border | tok.card.shadow   // composite tokens merge with |
.style = {.radius = lv::raw(3 * px)}                         // a deliberate, searchable exception
.style = {.radius = 8 * px}                                  // error: it takes a design token
```

Layout values (`.size`, `.range`, `.value`) and enums stay free. To accept plain values in styles,
change the project setting in the [settings file](docs/settings.md):

```json
{"rules": {"raw_style_values": true}}
```

Keep tokens in three tiers: primitive values (`primitive.color.blue-500`), semantic roles that alias
them (`color.primary`), and component tokens that alias those (`card.radius`). UI code uses the last two.

Styles in a mounted tree are stored as `const lv_style_t` in read-only memory.

A style for one part in one state is a selector, merged into the style with `|`:

```cpp
.style = base | lv::on("knob", "pressed", {.bg_color = tok.color.active})
             | lv::on("indicator", "checked", {.bg_color = tok.color.accent})
```

The names are LVGL's parts (`knob`, `indicator`, `items`, `cursor`, ...) and states (`pressed`, `checked`, `focused`, ...).
With one name it is a part in the default state, or a state of the main part: `lv::on("pressed", {...})` does what `.style_pressed` does.
A wrong name is a compile error that lists the right ones.
Each selector is a constant style in read-only memory, so it costs no heap beyond LVGL's entry in the widget's list of styles.

A theme gives every widget of a kind its styles, so a screen needs none of its own. It is one
constant, kept in read-only memory, and the last argument of a display:

```cpp
constexpr auto mine = lv::theme({
    .button = {.style = {.bg_color = tok.color.primary, .radius = tok.radius.none},
               .style_pressed = {.bg_color = tok.color.pressed}},
    .screen = {.style = {.bg_color = tok.color.ground}},
});

lv::display display{app, frame, mine};                // or lv::signal::dark, from <lv/signal.hpp>
```

[docs/themes.md](docs/themes.md) shows a whole one, and Signal, the theme that comes with lvgl26.

Signal has typefaces of its own, and `lv/signal_type.hpp` makes them. `lv::signal::dark_with_type` is the theme that sets its text in them, and `lv::signal::numerals` makes a font of digits for a large reading. They are in a second header because the compiler draws the fonts, which costs compile time and flash. The tell-tale lamp colours that the law defines are in a header of their own, `lv/signal_lamps.hpp`.

A font is made by the compiler from an embedded TrueType file: glyph pictures and kerning in
read-only memory, no heap and no converter. A typography token chooses it:

```cpp
constexpr unsigned char barlow_ttf[] = {
#embed "Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf}});
constexpr auto title = type(tok.text.title);          // family, weight and size from the token

lv::label({.style = {.text_font = title}, .text = "Wake up"})
```

Where the size is only known on the device, `lv::scaled_font` lets the device draw the glyphs, and a
style binds to it: `.text_font = lv::bind(title)`, then `title.resize(tok.text.large)`.

[docs/fonts.md](docs/fonts.md) has the costs and the limits.

A picture works the same way. The compiler decodes an embedded PNG file into the display's own
pixels, so the device needs no decoder:

```cpp
constexpr unsigned char logo_png[] = {
#embed "logo.png"
};

lv::image({.src = lv::image_file<logo_png>})
```

A photo or a large picture can stay a file that the device decodes: `lv::encoded_image<photo_jpg>`.

[docs/images.md](docs/images.md) says how a picture is stored and what it costs.

Rules to know:

- Designated initializers must be in alphabetical order (`.id`, `.range`, `.value`). This is a C++ rule.
- A misspelled or misordered member is reported as `could not convert '{...}' to props<"arc">`.
  GCC does not name the member in that case.
- `lv::widget<"spinbox">({...})` reaches any widget that has no short name.
- `view.set<"name">(...)` and `view.get<"name">()` are the untyped escape hatch to any `lv_*_set_*` / `get_*`.
  One thing is typed there: a getter of an angle returns an `lv::angle` or `lv::decidegrees`, so
  `image.get<"rotation">() == 90 * deg` holds whatever LVGL's number is.
- In a design token file an angle is a `number` token in degrees (`22.5`). In a run-time document it is
  `22.5` or `"22.5deg"`. A fraction is accepted only where LVGL takes tenths of a degree.

## In your editor

clangd cannot parse reflection, so lvgl26 gives it stub headers of the public API to read instead. Add `lvgl26_editor_support()` to your `CMakeLists.txt` and run CMake. Your editor then completes the members of `lv::arc({.`, shows what each one takes, and marks a wrong name in quotes. [docs/editor-support.md](docs/editor-support.md) says what works and what does not yet.

## Requirements

- GCC 16.2 or newer (`-std=c++26 -freflection`)
- No precompiled header that contains lvgl26's headers. With GCC 16.2 a precompiled header silently drops the precondition checks of templates ([GCC bug 128909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128909)).
- CMake 3.30 or newer, Ninja

## Build and test

```sh
cmake --preset host-v96 && cmake --build --preset host-v96 && ctest --preset host-v96
cmake --preset host-v10 && cmake --build --preset host-v10 && ctest --preset host-v10
```

| Preset | LVGL |
|---|---|
| `host-v96` | v9.6.0 |
| `host-v10` | master commit `53819fe4`, the v10 development line |

To build against a local LVGL checkout, add `-DFETCHCONTENT_SOURCE_DIR_LVGL=<path>`.

## On a microcontroller

The library builds with `-fno-exceptions -fno-rtti`, and `examples/mcu/slice.cpp` runs on an Arm
Cortex-M4 in QEMU: a compile-time tree, a label bound to a state, and a task.

```sh
brew install arm-none-eabi-binutils arm-none-eabi-gcc qemu   # GCC 16.2, without its libraries
tools/arm-toolchain.sh build/_tools/arm                      # builds newlib and libstdc++, once
tools/arm-run.sh                                             # builds the slice, runs it in QEMU
```

Measured: 269 KB of code with LVGL's default configuration, where every widget and feature is on.
The screen takes 1052 bytes of LVGL's heap and the task 128.
The application and the display objects are 1.6 KB of the code.
A program for a device should define `handle_contract_violation` itself, as the example does: the standard library's handler brings the exception run-time with it (12 KB).

## With Claude Code

The repository is also a Claude Code plugin. It gives an agent what is already known about LVGL, C++26, device UI standards and 3D, with sources. The agent then does not search for it again.
Its skill `write-ui` tells an agent how to write a screen with lvgl26: which page to read for each feature, which header a name needs, and what to do with an error.

```
/plugin marketplace add filipsajdak/lvgl26
/plugin install lvgl26@lvgl26
```

`python3 tools/check_plugin.py` checks the manifests and that every file a skill points to exists. More skills are planned: designing with the Signal system, and porting from another framework.

## Website

The public site is the plain HTML and CSS in `site/`: no build step and no dependencies.
`.github/workflows/pages.yml` publishes it to GitHub Pages on every push to `main`.

```sh
cd site && python3 -m http.server     # preview at http://localhost:8000
python3 tools/check_site.py           # every link, asset and anchor resolves
```

`site/status.html` lists what works today and what is planned. Update it with every feature.
