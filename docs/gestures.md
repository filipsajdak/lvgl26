# Swipes and holds

A finger that moves across a page is a swipe; a finger that stays on a widget is a hold. A swipe gives its direction as a value, to a handler or to a task that waits for it. A hold gives its progress in a state.

Both are in `#include <lv/gesture.hpp>`, and the pointer is in `#include <lv/pointer.hpp>` (see [Which header](which-header.md) and [Input](input.md)). Neither is part of `<lv/lv.hpp>`.

## The smallest example

A page that takes a swipe with a handler, and a task that waits for one:

<!-- snippet: tests/gesture.cpp page -->
<!-- snippet: tests/gesture.cpp waits -->

`.on_swiped = &on_swipe` is called with the direction, `lv::swipe::left`, `right`, `up` or `down`: the way the finger moved. `co_await lv::swiped(page)` gives the same value. The filter `lv::swiped<lv::swipe::left, lv::swipe::right>(page)` ignores the other directions and goes on waiting. As an arm of `lv::first` it is `lv::swiped(page).then([](lv::swipe d) { ... })`.

## How it works

- LVGL sends a swipe while the finger is still down, when it has moved the distance of the gesture limits, in the direction of the larger movement. A swipe that starts anywhere on the page counts, also on a button.
- It goes to the widget that was pressed if that keeps gestures, and otherwise to its first ancestor that does. Every widget passes gestures on, and a swipe that reaches no widget that keeps them is lost. A wait for a swipe, and `.on_swiped`, make their widget one that keeps them, so the nearest page that waits gets the swipe and a wait above it sees nothing.
- A drag that LVGL takes as a scroll is no swipe.

## Hold

<!-- snippet: tests/gesture.cpp hold -->

`co_await lv::hold(ring, 1 * sec, progress)` gives `true` when the finger stayed on the widget for the whole time, and `false` when it was lifted before, or the drag became a scroll. While it stays, `progress` goes from 0 to 100; when it leaves, it goes back to 0. A bar or an arc follows the state through `lv::computed`. If the finger is down when the wait begins, the time runs from then. The state must outlive the wait.

## The limits

The thresholds belong to the pointer. The defaults are LVGL's:

| Limit | Default | What it decides |
|---|---|---|
| `swipe_distance` | 50 px | how far the finger goes before a swipe is sent |
| `swipe_speed` | 3 px | how far it goes in each read of LVGL, 33 ms; a slower drag starts the count again |
| `scroll_limit` | 10 px | how far it goes before a drag is a scroll |
| `long_press` | 400 ms | the time of LVGL's own long press, one for the device; a hold has its own time |

A design keeps them as a token group `gesture`, and the pointer takes it: `lv::pointer mouse{display, tok.gesture}`. Without a group, `lv::gesture_limits{.swipe_distance = 30 * px}` is one with the other members at their defaults. Signal's design has the group with these values.

## What you can do

- Turn the swipes off for part of a page: `.gesture_bubble = false` on a widget that no wait names. It keeps the swipes of what lies in it, and nothing above it sees them. A brewing page does this, and still takes a hold.
- Let a list scroll up and down and keep the swipes sideways: `.scroll_dir = "ver"` on the list. A list with the default direction takes a sideways drag as an elastic scroll, and gives no swipe. A page that is no scroll container needs `.scroll_dir = "none"` for the same reason.

## What the compiler checks

The progress of a hold is a state of percents:

<!-- message: hold_progress_not_percent -->
```text
error: use of deleted function 'void lv::hold(const V&, lv::duration, lv::state<T>&) [with V = lv::view<lv::detail::fixed<4>{"obj"}>; T = int]': lv: the progress of lv::hold is an lv::state<lv::percent>: lv::state<lv::percent> progress{lv::percent{0}}. It goes from 0 to 100 while the finger stays.
```

## What it costs

- No scan of LVGL's names: a wait is an event handler on the widget, and a hold is one animation while the finger stays. A hold allocates one animation of LVGL while it runs; a swipe allocates nothing.
- `.on_swiped` is one more handler member of every widget in a props struct; it adds one name to the 70 or so handlers.

## Limits

- The swipe is sent while the finger is down, once for each press. A page that follows the finger needs the drag itself, which is not built.
- A wait or a handler on a page makes the page keep the swipes: a handler on the screen and one on a page inside it do not both fire; the page's does.
