# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

Documentation: start with the [quick start](docs/quick-start.md). The site is at <https://filipsajdak.github.io/lvgl26/>.

Status: early. Typed widget views, the declarative tree, styles, design tokens, state, derived state and coroutines exist. The first agent tools exist: a tree dump, screenshots and an MCP server (`lv/mcp.hpp`, `examples/agent`).

```cpp
#include <lv/lv.hpp>
using namespace lv::units;

void reset();

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
    lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];

auto root = lv::mount<ui>(display.screen());      // std::expected; owns the widgets
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
  tenths are taken. LVGL says which is which only in its doc comments, so lvgl26 reads them from the
  embedded headers at compile time (`include/lv/detail/angle_docs.hpp`), and a test checks the result
  against the pixels LVGL draws.
- **Mistakes are compile errors.** A wrong value says what the property takes and what you gave.
  A wrong id lists the ids of the tree.
- **The tree costs nothing at run time.** `lv::mount<ui>` reads it at compile time; an optimized build
  contains only the LVGL calls for the properties that are set.
- **Views never delete.** `lv::view<"arc">` is one pointer. `lv::mount` and `lv::owned<T>` own widgets;
  `lv::weak<T>` empties itself when LVGL deletes the widget.

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
  does not include mp-units; it accepts any type of that shape, so the dependency is yours to add.
  It builds freestanding, and a quantity compiles to the same code as an `int`.
- **A count:** `lv::counter`.
- **Anything else that is a number:** a struct with one integer, bool or enum member, such as
  `struct tab_index { int value; };`.
- A bool, an enum, a color or a pointer (`const char*` for text).

`lv::state<int>` is rejected, because a plain integer does not say what it is; the error shows these
choices. `lv::config::raw_integer_state` allows it.

A state is an `lv_subject_t` underneath (`speed.raw()`), so C code can share it. Where LVGL has a
bind function (`lv_arc_bind_value`), it is used; any other single-value property follows through a
generic observer.

A value made from states is an `lv::computed`, and an action that follows them an `lv::effect`.
The dependencies are part of the type, so both can be constant-initialized globals:

```cpp
constinit lv::state limit{50 * km / h};
constinit lv::computed over_limit{[](auto s, auto l) { return s > l; }, speed, limit};
constinit lv::effect log{[](auto s) { std::printf("%d\n", s.numerical_value_in(km / h)); }, speed};

lv::label({.hidden = lv::bind(over_limit), .text = "Within the limit"})   // bound like a state

log.start();                                           // runs now, and after every change
```

`over_limit.get()` calls the function with the current values; nothing is stored. Binding a computed
creates its `lv_subject_t` and one observer on each dependency. The result follows the rules of a
state, so a function that returns a plain `int` is rejected. See `include/lv/computed.hpp` for how
changes travel and who owns what.

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

Each token group becomes a struct and each token a typed `lv::token<T>`: colors, dimensions
(spacing, radius, sizes), durations, numbers (opacity), font family and weight, easing curves, and the
composite types shadow, border, typography, transition and gradient. Aliases are resolved. A misspelled
token, a broken alias or an unreadable value is a compile error.

**Styles use tokens.** A design value in a style (color, length, opacity, time, number) must be a token:

```cpp
.style = {.radius = tok.card.radius}                       // ok
.style = lv::style({...}) | tok.card.border | tok.card.shadow   // composite tokens merge with |
.style = {.radius = lv::raw(3 * px)}                         // a deliberate, searchable exception
.style = {.radius = 8 * px}                                  // error: it takes a design token
```

Layout values (`.size`, `.range`, `.value`) and enums stay free. To accept plain values in styles,
change the project setting. Settings are a C++ specialization, not a macro:

```cpp
template <>
inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};
```

Keep tokens in three tiers: primitive values (`primitive.color.blue-500`), semantic roles that alias
them (`color.primary`), and component tokens that alias those (`card.radius`). UI code uses the last two.

Styles in a mounted tree are stored as `const lv_style_t` in read-only memory.

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

## Requirements

- GCC 16 or newer (`-std=c++26 -freflection`)
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
The screen takes 1052 bytes of LVGL's heap and the task 128. The application and the display
objects are 1.6 KB of the code. A program for a device should define
`handle_contract_violation` itself, as the example does: the standard library's handler brings the
exception run-time with it (12 KB).

## With Claude Code

The repository is also a Claude Code plugin. It gives an agent what is already known about LVGL, C++26, device UI standards and 3D, with sources. The agent then does not search for it again.

```
/plugin marketplace add filipsajdak/lvgl26
/plugin install lvgl26@lvgl26
```

`python3 tools/check_plugin.py` checks the manifests and that every file a skill points to exists. More skills are planned: building a screen, designing with the Signal system, and porting from another framework.

## Website

The public site is the plain HTML and CSS in `site/`: no build step and no dependencies.
`.github/workflows/pages.yml` publishes it to GitHub Pages on every push to `main`.

```sh
cd site && python3 -m http.server     # preview at http://localhost:8000
python3 tools/check_site.py           # every link, asset and anchor resolves
```

`site/status.html` lists what works today and what is planned. Update it with every feature.
