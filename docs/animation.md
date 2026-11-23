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

## How the value moves

A fourth argument says how the value moves:

- a name from `lv::ease`: LVGL's own paths and named curves, such as `lv::ease::in_quad`, `out_back` and `bounce`;
- a `cubicBezier` design token, such as `*tok.motion.fall` above;
- an `lv::bezier`, as in CSS.

Without it the value moves at one speed.

## When it ends

The result is `true` when the animation ran to its end. It is `false` when something stopped it: a newer animation of the same property took over, or LVGL's `lv_anim_delete()` removed it.

Cancelling the task, or losing in `lv::first`, deletes the animation.
