# Values and units

Every value a property takes is typed: a length has a unit, an angle is in LVGL's unit, and an LVGL value is a name.

Header: `<lv/lv.hpp>`, the core.

## Typed values

| A property takes | Write |
|---|---|
| A size or a position | `120 * px`, `50 * pct` or `lv::size_content` |
| An angle | `90 * deg` |
| A time | `300 * ms` |
| An opacity | `50 * pct` |
| A whole number | an `int` that fits; `bool`, floating point and a value that does not fit are refused |

A wrong value stops the build and says what the property takes:

<!-- message: props_bool_as_int -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a whole number (not bool, not floating point). You gave bool.
```

## Measures of the application

A value of the application with a unit is an [mp-units](https://github.com/mpusz/mp-units) quantity, such as a speed in a [state](state.md):

```cpp
constinit lv::state speed{0 * km / h};

speed = 120 * km / h;  // the widgets update
speed = 2 * km / min;  // converted: nothing is lost
speed = 33 * m / s;    // error: 118.8 km/h does not fit an int
speed = 3 * m;         // error: not a speed
```

lvgl26 has no unit system of its own and does not include mp-units. It takes any type of that shape: a quantity with an integer number.

`set(LVGL26_UNITS ON)` in CMake fetches mp-units for you, as the [quick start](quick-start.md) does. It builds freestanding, and a quantity compiles to the same code as an `int`.

## Angles

LVGL takes most angles in whole degrees: an arc, a scale. That is an `lv::angle`.

An image's rotation, and a style's `transform_rotation`, `transform_skew_x` and `transform_skew_y`, take tenths of a degree. That is an `lv::decidegrees`.

Write `90 * deg` for both. `22.5 * deg` compiles only where LVGL takes tenths.

LVGL says which unit a setter takes only in its doc comments. lvgl26 reads them from LVGL's headers at compile time (`include/lv/detail/angle_docs.hpp`). A test checks the result against the pixels LVGL draws.

A setter that takes its angle unsigned, such as a spinner's `.arc_sweep = 270 * deg`, takes an `lv::nonnegative<lv::angle>`. A negative constant there is a compile error, and a negative value at run time breaks a contract.

In a design token file an angle is a `number` token in degrees, such as `22.5`. In a run-time document it is `22.5` or `"22.5deg"`. A fraction is accepted only where LVGL takes tenths of a degree.

A getter of an angle returns an `lv::angle` or an `lv::decidegrees`, so `image.get<"rotation">() == 90 * deg` holds whatever LVGL's number is ([Views and ids](views.md)).

## Names of LVGL's values

An LVGL enumerator is its name in quotes: `.align = "center"`, `.flex_flow = "row_wrap"`, `.flex_align = {"center", "center", "start"}`.

The name is the enumerator without the part that all of its enum share, in lower case. So `LV_ALIGN_TOP_LEFT` is `"top_left"`.

An internal enumerator, which LVGL marks with an underscore, keeps its whole name. It does not shorten the names of the others.

A name may stand beside values of other kinds: a roller takes `.options = {{"Mon", "Tue"}, "infinite"}`.

A wrong name stops the build and lists the right ones:

<!-- message: enumerator_unknown_name -->
```text
error: constexpr message: lv: align has no value 'centre'. Its values: default, top_left, top_mid, top_right, bottom_left, bottom_mid, bottom_right, left_mid, right_mid, center, out_top_left, out_top_mid, out_top_right, out_bottom_left, out_bottom_mid, out_bottom_right, out_left_top, out_left_mid, out_left_bottom, out_right_top, out_right_mid, out_right_bottom.
```
