# Values and units

Every value a property takes is typed: a length has a unit, an angle is in LVGL's unit, and an LVGL value is a name.

Header: `<lv/lv.hpp>`, the core.

## The smallest example

Two arcs of LVGL's examples, each turned by an angle in degrees, with a size in pixels and a label placed by name:

<!-- snippet: examples/lvgl/widgets/arc_rotation.cpp -->
```cpp
#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The same three-quarter arc, turned by 45 and by 135 degrees.
constexpr auto turned_45 = lv::arc(
  {
    .bg_end_angle   = 270 * deg,
    .bg_start_angle = 0 * deg,
    .rotation       = 45 * deg,
    .size           = {120 * px, 120 * px},
    .value          = 60,
  },
  lv::label({.align = "center", .text = "rot=45"})
);

constexpr auto turned_135 = lv::arc(
  {
    .bg_end_angle   = 270 * deg,
    .bg_start_angle = 0 * deg,
    .rotation       = 135 * deg,
    .size           = {120 * px, 120 * px},
    .value          = 60,
  },
  lv::label({.align = "center", .text = "rot=135"})
);

}  // namespace

// The screen itself is the row, as in the C example: the gap between the arcs is
// the one the theme gives a screen. A tree has one root, so each arc is its own.
void widgets::arc_rotation(lv::obj screen) {
  screen.set({.flex_flow = "row", .style = spread_out});
  lv::place<turned_45>(screen);
  lv::place<turned_135>(screen);
}
```

`using namespace lv::units` brings `px`, `pct`, `deg`, `ms` and the rest.

## How it works

Each member of a props struct has the type of what its setter takes, worked out by reflection from LVGL's C API. A length is an `lv::length`, a time an `lv::duration`, an angle an `lv::angle` or an `lv::decidegrees`. A number without its unit is refused.

LVGL says which unit an angle takes only in its doc comments. lvgl26 reads them from LVGL's headers when it compiles, and a test checks the result against the pixels LVGL draws.

An LVGL enumerator is its name in quotes. The compiler finds the enumerator of that name in the property's enum, and refuses a name it does not have.

A value of the application with a unit is an [mp-units](https://github.com/mpusz/mp-units) quantity. lvgl26 has no unit system of its own, and takes any type of that shape: a quantity with an integer number.

## What you can do

### Give a size, a position, a time, an opacity

| A property takes | Write |
|---|---|
| A size or a position | `120 * px`, `50 * pct` or `lv::size_content` |
| An angle | `90 * deg` |
| A time | `300 * ms` |
| An opacity | `50 * pct` |
| A whole number | an `int` that fits; `bool`, floating point and a value that does not fit are refused |

### Give an angle

Most angles of LVGL are whole degrees: an arc, a scale. That is an `lv::angle`.

An image's rotation, and a style's `transform_rotation`, `transform_skew_x` and `transform_skew_y`, take tenths of a degree. That is an `lv::decidegrees`. Write `90 * deg` for both. `22.5 * deg` compiles only where LVGL takes tenths.

A setter that takes its angle unsigned, such as a spinner's `.arc_sweep = 270 * deg`, takes an `lv::nonnegative<lv::angle>`. A negative constant there is a compile error, and a negative value at run time breaks a contract.

In a design token file an angle is a `number` token in degrees, such as `22.5`. In a run-time document it is `22.5` or `"22.5deg"`.

### Name an LVGL value

`.align = "center"`, `.flex_flow = "row_wrap"`, `.flex_align = {"center", "center", "start"}`. The name is the enumerator without the part that all of its enum share, in lower case: `LV_ALIGN_TOP_LEFT` is `"top_left"`.

An internal enumerator, which LVGL marks with an underscore, keeps its whole name, and does not shorten the names of the others. A name may stand beside values of other kinds: a roller takes `.options = {{"Mon", "Tue"}, "infinite"}`.

### Give a value of the application its unit

A state holds a quantity with its unit: here `beans` holds grams. A state of a plain `int` is refused, because the number does not say what it is.

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

`set(LVGL26_UNITS ON)` in CMake fetches mp-units for you, as the [quick start](quick-start.md) does. mp-units builds freestanding, and a quantity compiles to the same code as an `int`. [State and bindings](state.md) says what else a state holds.

## What the compiler checks

A number where a length belongs:

<!-- message: props_needs_unit -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.
```

A fraction of a degree where LVGL takes whole degrees:

<!-- snippet: tests/fail/props_fraction_for_whole_angle.cpp -->
```cpp
#include <lv/lv.hpp>
using namespace lv::units;
constexpr auto ui = lv::arc({.rotation = 22.5 * deg});
```

<!-- message: props_fraction_for_whole_angle -->
```text
error: static assertion failed: lv: wrong value for this property. It takes an angle in whole degrees (90 * lv::deg). You gave lv::decidegrees.
```

A `bool` where a whole number belongs:

<!-- message: props_bool_as_int -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a whole number (not bool, not floating point). You gave bool.
```

These stop the build too, each with a test of its own in `tests/fail/`:

- a floating-point number for a whole number (`props_narrowing`), and a length for an angle (`props_wrong_unit`);
- tenths of a degree that are no whole tenth, or too large (`units_decidegrees_not_tenths`, `units_decidegrees_too_large`);
- a state of whole degrees bound to a property in tenths (`bind_angle_to_decidegrees`);
- a name that the property's enum does not have (`enumerator_unknown_name`, shown in [The tree and its widgets](tree.md)).

## What it costs

| Cost | What we know | Where it comes from |
|---|---|---|
| Processor time | not measured yet | not measured yet |
| RAM | not measured yet | not measured yet |
| ROM | not measured yet | not measured yet |
| Stack | not measured yet | not measured yet |
| Compile time | not measured yet | not measured yet |

## Rules

<!-- rules: checked-1 checked-2 -->
- [`checked-1`](rules.md#checked-1): Give every value a type that says what it is: a quantity, an `lv::counter`, or a struct of one member.
- [`checked-2`](rules.md#checked-2): Write an LVGL enumerator as its name in quotes, `.align = "center"`; a misspelled name stops the build and lists the right ones.
<!-- rules: end -->

## Limits

- An absolute temperature, an mp-units quantity point such as `point<deg_C>(92)`, is refused by a state: it holds a quantity.
- A property that takes a whole number takes no percentage, though LVGL writes some as `LV_PCT(80)`.
- A text binding with a format shows a quantity's number without its unit, so the format writes the unit.

What is planned is on the [status page](https://filipsajdak.github.io/lvgl26/status.html).

## Names

`<lv/lv.hpp>` declares `lv::length`, `lv::angle`, `lv::decidegrees`, `lv::nonnegative`, `lv::duration` and the units of `lv::units`, as [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) lists them. What each property takes, and the names of each enum, are in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json).
