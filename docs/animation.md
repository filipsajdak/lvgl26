# Animation

`lv::animate` moves one property of a widget from one value to another, and a task can wait for it to end.

Header: `<lv/lv.hpp>`, the core.

## A fall and a rise

The ball of `examples/bounce` falls and rises in tasks that each await one animation, eased by curves from the token file:

<!-- snippet: examples/bounce/scene.hpp fall -->
```cpp
// One fall from `height`, or one rise to it.
inline lv::task<> fall(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(
    box_view, {.y = {(landing - height) * px, landing * px}}, fall_time(height), *tok.motion.fall
  );
  squash = 100 * height / drop_height;
}
inline lv::task<> rise(lv::view<"obj"> box_view, int height) {
  co_await lv::animate(
    box_view, {.y = {landing * px, (landing - height) * px}}, fall_time(height), *tok.motion.rise
  );
}
```

The [task](tasks.md) that calls them is the whole scene: a ball that bounces lower each time and squashes, with a shadow under it. `tests/example_bounce.cpp` steps its clock and pins the motion, and its pictures at fixed times.

## What can move

```cpp
lv::task<> open(lv::view<"arc"> arc) {
  co_await lv::animate(arc, {.value = {0, 100}}, 300 * ms);
  bool finished = co_await lv::animate(arc, {.width = {80 * px, 120 * px}}, 200 * ms);
}
```

`lv::animatable<"arc">` lists the properties that take one number: a whole number, a length, an angle, an opacity. It is generated from the same setters as `lv::props<"arc">`.

Both ends have the property's type. `.width = {0, 100}` is a compile error, and so is `{10 * px, 50 * pct}`.

## Style values: opacity, translation, scale

The plain call moves the local style values of the main part too: every style property that takes one number, each with its own type. `lv::animatable<"label">` lists them after the widget's own.

```cpp
lv::task<> appear(lv::view<"label"> label) {
  co_await lv::animate(label, {.opa = {0 * pct, 100 * pct}}, 220 * ms, lv::ease::out_quad);
  co_await lv::animate(label, {.translate_y = {0 * px, -24 * px}}, 220 * ms);
}
```

`lv::in<"part", "state">(view)` names the part and the state whose local style value the animation moves, as `lv::on` names them for a style. One name is a part or a state (`lv::in<"indicator">`, `lv::in<"pressed">`); two are the part and then the state. Behind `lv::in` the struct is `lv::animatable_style<>`: every style property that takes one number is a member, `width` and `height` included.

```cpp
lv::task<> press(lv::view<"arc"> arc) {
  co_await lv::animate(lv::in<"indicator">(arc), {.arc_width = {2 * px, 8 * px}}, 120 * ms);
  co_await lv::animate(lv::in<"knob", "pressed">(arc), {.opa = {20 * pct, 100 * pct}}, 120 * ms);
}
```

- A name that is both a widget setter and a style property (`width`, `height`, `x`, `y`) is the widget's in the plain call. The style value is reached through `lv::in`.
- The animation sets `from` at once, as it does for a widget setter; it does not read what a theme or a look gives. The value stays at `to`, or where it was when the animation was deleted, and a local value beats a theme, a look and `.style`: nothing but another local value changes it afterwards.
- Two selectors are two animations that run side by side. Two animations of one property in one selector replace each other, and `{.opa = ...}` and `lv::in<"main">(view)` are one selector.
- Colors, fonts, enums and pointers are not numbers: they are no members. A shorthand such as `pad_all` is one number for LVGL, and sets four properties: do not animate it and one of its parts together.
- Known limit: nothing removes a local value that an animation has set. It stays, and beats the theme, until another local value replaces it.
- `lv::in` costs a file nothing until it calls `lv::animate`; the properties are found by reflection from LVGL's `lv_obj_set_style_*` functions.

## How the value moves

A fourth argument says how the value moves:

- a name from `lv::ease`: LVGL's own paths and named curves, such as `lv::ease::in_quad`, `out_back` and `bounce`;
- a `cubicBezier` design token, such as `*tok.motion.fall` above;
- an `lv::bezier`, as in CSS.

Without it the value moves at one speed.

## When it ends

The result is `true` when the animation ran to its end. It is `false` when something stopped it: a newer animation of the same property took over, or LVGL's `lv_anim_delete()` removed it.

Cancelling the task, or losing in `lv::first`, deletes the animation.
