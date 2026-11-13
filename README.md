# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

Status: early. Typed widget views, the declarative tree, styles, design tokens, state and coroutines exist. Agent tools do not yet.

```cpp
#include <lv/lv.hpp>
using namespace lv::units;

void reset();

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
    lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];

auto root = lv::mount<ui>(lv::screen_active());   // std::expected; owns the widgets
root->get<"speedo">().set({.value = 0});          // a typed view of the arc
```

How it works:

- **Properties are generated.** `lv::props<"arc">` is a struct that C++26 reflection builds from LVGL's
  own C API: one member per `lv_arc_set_*` and `lv_obj_set_*` function and one per event (`on_clicked`).
  Nothing is written by hand and nothing is generated offline.
- **Properties are typed.** Sizes take `120 * px`, `50 * pct` or `lv::size_content`; angles `90 * deg`; times
  `300 * ms`; opacity `50 * pct`. Integers reject `bool`, floating point and values that do not fit.
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

Choosing by value is checked at compile time, in the shape of the pattern matching proposed for
C++29 (P2688):

```cpp
enum class mode { off, eco, sport };

const char* name = lv::match(m)(
    lv::is<mode::off>   >> [] { return "Off"; },
    lv::is<mode::eco>   >> [] { return "Eco"; },
    lv::is<mode::sport> >> [] { return "Sport"; });
// leave one out: error: lv::match does not cover: sport. Add an arm for each, or ...

int level = lv::match(n)(lv::is<1, 2> >> [] { return 0; }, lv::otherwise >> [] { return 1; });
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
