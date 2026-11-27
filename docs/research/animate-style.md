# lv::animate for style properties: design

Status: decided by the owner (2026-11-26/27, in the brief of the team lead); this page settles how it is built and lists what is still open. Marks: `source` is read in this repository, `reasoned` is mine.

## What is asked

1. The call of today moves local style properties of the main part too: `co_await lv::animate(view, {.opa = {0, 255}, .translate_y = {0 * px, -24 * px}}, time, ease)`. Reflection lists the style properties that take one number, as it does for widget setters. Types, ends and units stay checked.
2. A part and a state are written with a wrapper that reads like `lv::on`: `co_await lv::animate(lv::in<"indicator", "pressed">(view), {.opa = {...}}, 200 * ms)`. The signature of `lv::animate` stays; `lv::in<...>(view)` always means style properties.
3. A name that is a widget setter and a style property (`width`, `height`, `x`, `y`, `size`, `align`): `.width` in a plain call stays the widget setter. The style form is reached only through `lv::in`.
4. Both ends keep the property's type (`lv::opa`, `lv::length`, `lv::angle`). When the task is cancelled or loses in `lv::first`, the animation is deleted and the value stays.

## How LVGL animates a local style value

- A local style value is set by `lv_obj_set_style_<name>(obj, value, selector)`, where the selector is a part and a state or-ed (`LV_PART_INDICATOR | LV_STATE_PRESSED`). `lv::style` already holds these setters, found by reflection (`style_setters<D>`, `include/lv/style.hpp`), and `lv::on` turns two names into a selector (`part_named`, `state_named`, `part_or_state_named`). `source`
- `lv_anim_t` calls `exec_cb(var, value)` with the widget as `var` and a number: no selector. So the selector is a template argument of the step function, `style_step<Setter, Selector>`, as `animation_step<Setter>` is for a widget setter. `reasoned`
- LVGL identifies a running animation by `var` and `exec_cb`, and a new one deletes the old (`animation_source`'s comment). With one instantiation of the step for each setter and selector, two animations of one property in two parts or two states run side by side, and two of one property in one selector replace each other: what a user expects. The plain `{.opa = ...}` and `lv::in<"main">(view)` are one instantiation (the selector 0), so they replace each other. `reasoned`
- The first step sets `from` at once, as for a widget setter (`animation_source::arm`). A property with no local value yet therefore starts from `from`, not from the value that the theme or a look gives; the animation does not read the current value. `reasoned`
- The local value stays at `to` when the animation ends and at the last value when it is deleted. A local value beats every added style (a theme, `start_style`, a look, `.style`), so after the first animation of a property nothing but another local value changes it. This is how a widget's own setter works too, and no API removes a local value here. `source` for the order of precedence (LVGL's style rule, `docs/research/lvgl-upstream.md`)

## The API

```cpp
co_await lv::animate(label, {.opa = {0, 255}}, 220 * lv::ms, lv::ease::out_quad);                  // main part
co_await lv::animate(lv::in<"indicator">(arc), {.arc_width = {2 * px, 8 * px}}, 120 * lv::ms);     // a part
co_await lv::animate(lv::in<"pressed">(button), {.transform_scale = {256, 280}}, 120 * lv::ms);    // a state of main
co_await lv::animate(lv::in<"knob", "pressed">(slider), {.opa = {0, 255}}, 200 * lv::ms);         // both
```

- `lv::in<Names...>(view)` is a `lv::styled_view<W, Selector>` (the view and a selector). One name is a part or a state, as in `lv::on("knob", ...)` and `lv::on("pressed", ...)`; two are the part and then the state. The messages are those of `lv::on`, with `lv::in` in place of `lv::on` in the hint. `reasoned`
- Plain call: `animatable<W>`, the struct of `lv::animate`, gains the style properties after the widget's own: a member of the style struct that takes one number and has no widget setter of that name. A name that is both stays the widget setter. `lv::animatable_style`, a second struct without the widget members, is what `lv::in` takes: every style property that takes one number, `width` and `height` included.
- The numbers that a style property takes: `lv::opa`, `lv::length`, an angle (`lv::angle` or `lv::decidegrees`), a whole number. A color, a font, an enum, a bool or a pointer is not a number: no member, and the usual message of a member that does not exist. Shorthands (`pad_all`, `margin_hor`) are setters of one number and are listed; they set several properties, so animating one of them and one of its parts at once makes them fight. The docs say so.
- Both ends are checked as for widget setters (`can_run_between`): lengths in one scale, a whole number in range of 32 bits.
- `co_await` gives true when the animation ran to its end and false when another took over, and a stopped wait ends in the next turn of the loop: as it does.

## Cost

- The scan of the style struct is behind a template parameter, as `animatable_of<W, D>` is for the setters (`GCC 16.2 runs a call with constant arguments in a function that is no template where the header is read`, CLAUDE.md). A file that does not call `lv::animate` pays nothing.
- A file that animates pays for one more scan of the 139 style members and for the instantiations of its calls. It is measured by a new probe, `animate_style` (a file that animates one widget property and one style property), and the other probes must not move. The compile-cost table is in the PR.
- `lv::include_only` budget: `animate.hpp` is not part of `<lv/lv.hpp>`; the budget does not change.

## Open (asked of the team lead)

1. Does `lv::in` take one name as `lv::on` does (`lv::in<"indicator">(view)`, `lv::in<"pressed">(view)`), or always part then state?
2. Does the plain call carry the style properties inside `lv::animatable<W>` (what I propose), or is the plain call a second struct, so that `lv::animatable<W>` keeps listing the widget's setters only?
3. Confirm that no way to remove the local value afterwards is wanted now (the value stays, as the brief says for cancel).
