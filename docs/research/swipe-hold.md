# Swipe and hold: how LVGL gives them, and the API to build

Status: a note for the owner (2026-11-27). Nothing is built. Gaps 4 and 7 of the coffee machine plan (`build/coffee-assets/plan.md`, decisions 7 and 8).
Marks: `source` is read in LVGL v9.6.0 (`build/host-v96/_deps/lvgl-src`) or in this repository, `reasoned` is mine, `unmeasured` is what a test must show before the API is fixed.

## What is asked

1. A typed swipe: its direction is a value, usable in a handler and as `co_await` in a task, as the other waits are.
2. Hold to cancel: `lv::hold(view, duration, progress_state)`, true when the finger stayed down for the whole time, false when it was lifted, with the progress in a state that a bar or an arc follows.
3. Thresholds are design tokens.
4. The conflict with scrolling is settled and tested.

## How LVGL produces a gesture

- The gesture is sent while the finger is down, not on release. `indev_gesture()` runs on each read that moves a pressed pointer. It adds the movement of each read to `gesture_sum`; a read whose movement is below `gesture_min_velocity` in both axes sets the sum back to zero. When the sum passes `gesture_min_distance` on an axis, it sends `LV_EVENT_GESTURE` and then the direction event, once for this press (`gesture_sent`). `source`: `src/indev/lv_indev.c`, `indev_gesture`.
- The direction is that of the axis with the larger sum: `LV_EVENT_GESTURE_LEFT`, `_RIGHT`, `_UP`, `_DOWN`. Right means the finger moved right. The direction is also `lv_indev_get_gesture_dir(indev)` during the event. `source`.
- The event goes to the pressed widget if that does not pass gestures on (`LV_OBJ_FLAG_GESTURE_BUBBLE` is off), and otherwise to its first ancestor that does not. Every widget passes gestures on by default, so when none in the chain keeps them, `gesture_obj` ends as null and the swipe is lost. (Corrected on 2026-11-27, with the first build: the note first said that the event reaches the screen.) A wait for a swipe or a handler on a page must therefore clear the flag on its widget. `source`, `src/indev/lv_indev.c`, `indev_gesture`.
- The defaults, per input device: `gesture_min_distance` 50 px, `gesture_min_velocity` 3 px for each read (a read comes every 33 ms, so about 90 px/s), settable with `lv_indev_set_gesture_min_distance` and `lv_indev_set_gesture_min_velocity`, both 8 bits. `source`: `lv_conf_template.h`, `src/indev/lv_indev.c`.
- No gesture is sent while a scroll is running (`if (indev->pointer.scroll_obj) return;`). So a drag that LVGL takes as a scroll never becomes a swipe. `source`.

## A swipe also clicks, unless its handler ends the press (2026-11-28)

- On release LVGL sends `LV_EVENT_SHORT_CLICKED` and `LV_EVENT_CLICKED` to the pressed widget whenever no scroll started, also when a gesture was sent during the press: `indev_proc_release` checks `scroll_obj` and not `gesture_sent`. With press lock (the default) it does so also when the finger left the widget. `source`: `src/indev/lv_indev.c`, `indev_proc_release`, v9.6.0 and the v10 pin.
- Measured with plain LVGL (a pointer read callback, a 300 x 100 button, the screen not scrollable, 20 px a read for 6 reads), the same on v9.6.0 and on the v10 pin: a tap gives 1 click; a swipe inside the button gives 1 click and 1 gesture, whether the screen or the button takes the gesture; a swipe that leaves the button gives 1 click; with `lv_indev_wait_release(lv_indev_active())` in the gesture handler, 0 clicks, 0 releases, and the button gets `LV_EVENT_PRESS_LOST`. `measured`.
- It is the intended behaviour. The gesture docs say: "If you performed some action on a gesture, you can call `lv_indev_wait_release(lv_indev_active())` in the event handler to prevent LVGL from sending further input-device-related events" (`docs/src/main-modules/indev/gestures.mdx`, v9.6.0 and master). The maintainer chose this in [lvgl#3211](https://github.com/lvgl/lvgl/issues/3211) (2022): LVGL cannot know whether the program wanted the gesture, so the handler that takes it says so. `source`.
- So lvgl26 ends the press when a wait for a swipe (`lv::swiped`, a wanted direction) or `.on_swiped` takes it (`detail::take_swipe` in `include/lv/props.hpp`); `tests/gesture.cpp`, `a_taken_swipe_clicks_nothing`. The generated `.on_gesture_left` and its siblings stay LVGL's plain events. `measured`.

## How LVGL decides that a drag scrolls

- After the pointer moved `scroll_limit` pixels (10 by default) from the press, `lv_indev_find_scroll_obj` looks for an object to scroll: the pressed widget and its ancestors, the first that can scroll in the direction of the drag (it has content beyond its edge on that axis). If none can scroll in that direction but one can scroll on that axis, it is the candidate and shows the elastic effect. If none can scroll on that axis, there is no scroll object and the drag stays a candidate for a gesture. `source`: `src/indev/lv_indev_scroll.c`.
- A widget that is a scroll container in an axis (`scroll_dir` allows it and the flag is on) is a candidate even when it has nothing to scroll: the drag is an elastic scroll and no swipe (measured in `tests/gesture.cpp`, 2026-11-27). A list that scrolls only up and down needs `scroll_dir` `ver` for sideways swipes, and a page that scrolls nothing needs `none`.
- So a swipe across a page that scrolls only vertically works horizontally, and a vertical swipe on it scrolls. A swipe down for the Service page works only on a page that does not scroll vertically, or whose scroll is at its end and not elastic. `reasoned` from the source; `unmeasured`.
- When a scroll starts, LVGL removes the pressed state of the widget under the finger and sends no more events of the press that a hold would use (`LV_EVENT_LONG_PRESSED` is sent only `if scroll_obj == NULL`). `source`.

## How LVGL produces a long press

- `LV_EVENT_LONG_PRESSED` is sent once, when the press has lasted `long_press_time` (400 ms by default, one value for the device: `lv_indev_set_long_press_time`) and no scroll has started. `LV_EVENT_LONG_PRESSED_REPEAT` follows every `long_press_repeat_time` (100 ms). There is no progress, and the time cannot differ between two widgets. `source`.
- `LV_EVENT_PRESSED`, `LV_EVENT_PRESSING` (every read while down), `LV_EVENT_RELEASED` and `LV_EVENT_PRESS_LOST` (the finger slid off the widget) give what a hold needs: the start, the end and an abnormal end. `source`: `include/lvgl/core/lv_event.h`.

## What lvgl26 has today

- `view.next().gesture_left` (and `_right`, `_up`, `_down`, `long_pressed`, `pressed`, `released`, `press_lost`) are generated from `lv_event_code_t` (`include/lv/events.hpp`) and can be awaited and used with `lv::first(...then(...))` today. A handler property `.on_gesture_left` exists the same way. So the direction events are reachable; what is missing is a value for the direction, tokens for the thresholds, a hold with a progress, and the tests for the rules above. `source`.
- `lv::pointer` owns the LVGL input device (`include/lv/pointer.hpp`), feeds it from a queue and reads at the next step, so a test can drive a whole drag with samples and ticks. It sets no threshold. `source`.
- A full queue merges a move into the last sample. A merged move keeps the sum of the movement of the read, but a slow platform that merges many moves into one read can make a read's movement large: a fast flick and a slow drag differ in the velocity rule. `unmeasured`.

## The swipe API: alternatives

```cpp
enum class swipe { left, right, up, down };          // the direction the finger moved
```

| | A. One wait that gives the direction | B. One wait for each direction | C. Handlers only |
|---|---|---|---|
| Wait | `const lv::swipe d = co_await lv::swiped(root);` | `co_await lv::swiped_left(root)` (a named `lv::first` arm each) | none |
| `lv::first` | `lv::swiped(root).then([](lv::swipe d) {...})` | `lv::swiped_left(root).then(back)`, `lv::swiped_right(root).then(next)` | none |
| Handler | `.on_swiped = &on_swipe` with `void on_swipe(lv::swipe)` | `.on_swiped_left` (today's `.on_gesture_left`) | the same |
| New code | one event source (LV_EVENT_GESTURE, direction read from the device) | renames of generated events | none |
| A filter | `lv::swiped<lv::swipe::left, lv::swipe::right>(root)` takes only those, ignores the rest | not needed | not needed |

My recommendation is A with the filter: one thing to learn, a typed value a `lv::match` can take (a state of `lv::swipe`), and the filter keeps a page that wants only two directions from waking for the others. B costs nothing and exists today under other names. C is today.

## The hold API: alternatives

```cpp
// true: held to the end. false: released, slid off, or the drag became a scroll.
const bool done = co_await lv::hold(ring, 1000 * lv::ms, progress);   // progress: a state the ring follows
```

| | A. Built on press events and an animation | B. Built on `LV_EVENT_LONG_PRESSED` | C. A timer of our own |
|---|---|---|---|
| Start | `LV_EVENT_PRESSED` of the view | the device's long press time | `LV_EVENT_PRESSED` |
| Progress | an `lv_anim_t` from 0 to the full value, writing the state | none | an `lv_timer_t` every 33 ms |
| End | the animation ends: true; `RELEASED`, `PRESS_LOST` or scroll begin: false, animation deleted | the event: true | the timer passes the time: true |
| Time | any, for each call | the device's one value | any |
| In tests | ticks advance the animation as any other | ticks | ticks |
| Heap | one animation while held, like `lv::animate` | none | one timer while held, like `lv::sleep` |

I recommend A: it uses the machinery of `lv::animate`, so its cancellation (a task that ends, a widget that is deleted, a `lv::first` that loses) is already right, and progress is smooth at the display's refresh. B cannot show progress and cannot differ per widget.

The progress state: a state of what? `lv::state<lv::percent>` would read as 0 to 100 and a bar takes it; an arc takes 0 to 360 degrees; `lv::opa` is 0 to 255. A new strong value `lv::progress` (0 to 1000) is exact. My recommendation: `lv::state<lv::percent>` of whole percents, because a bar and an arc range already take a percent in a computed.

## Thresholds as tokens

| Threshold | LVGL setting | Token |
|---|---|---|
| distance of a swipe | `gesture_min_distance`, 50 px | `gesture.swipe-distance`, a length |
| speed of a swipe | `gesture_min_velocity`, 3 px for each read | `gesture.swipe-speed`, a length for each 33 ms read, written as `{"value": 3, "unit": "px"}` with the read period fixed in the docs |
| distance before a scroll | `scroll_limit`, 10 px | `gesture.scroll-limit`, a length |
| time of LVGL's long press | `long_press_time`, 400 ms | `gesture.long-press`, a duration (the hold has its own time per call) |

The device takes them in its constructor: `lv::pointer mouse{display, {.swipe_distance = tok.gesture.swipe_distance, ...}}`, so one place in the program holds them and a test can change them. Signal gets a `gesture` group in its design copy (`tools/signal_sync.py --record`, `tools/signal_tokens.py`), as it got motion, with the values of LVGL's defaults, so that a design can change them in Claude Design. `reasoned`.

## The scroll conflict: rules to document and test

1. A swipe is sent only when no scroll started. A page that scrolls on an axis loses its swipes on that axis.
2. A page with no overflow has no scroll object, so every direction is a swipe on it. The coffee pages have none but Service, so back and next work everywhere, and "down for Service" works on every page but Service.
3. A hold is cancelled by a scroll (LVGL removes the pressed state); the wait gives false.
4. "Swipes off while brewing" is the absence of a waiter: a page that does not wait for a swipe or set `.on_swiped` ignores it, and the event reaches no handler. A shell that waits for swipes on the screen would see them on every page, so the shell waits per page and not once for the screen. `reasoned`.
5. A widget that wants no gesture to leave it (a slider the finger drags) clears `LV_OBJ_FLAG_GESTURE_BUBBLE`; a drag on a slider is not a scroll, and its own handling comes first. A test of a slider inside a swipe page decides whether the library clears it for the widgets that take a drag. `unmeasured`.

## Tests to write first

- `swipe_direction`: a drag of 60 px in each direction, in reads of 8 px, sends one event of the right direction; a drag of 40 px sends none; a drag of 60 px with a pause of one read (movement below the speed) sends none.
- `swipe_scroll`: on a page that scrolls vertically a vertical drag scrolls and sends no swipe; a horizontal one sends a swipe.
- `swipe_task`: `co_await lv::swiped(root)` gives the direction and ends with the owner.
- `hold_complete`: the finger stays down 1 s: true, progress ends at the full value. `hold_release`: lifted at 400 ms: false, the animation is gone, nothing left on LVGL's heap (`full_heap.hpp` as in the other task tests). `hold_slide_off`: false. `hold_scroll`: false.
- Compile-fail tests for the messages: a threshold that is no length token, a hold with a state of the wrong type.

## Decisions of the owner (2026-11-27)

1. One wait gives the direction, with the filter by template arguments: `co_await lv::swiped(root)`, `co_await lv::swiped<lv::swipe::left, lv::swipe::right>(root)`, and the handler `.on_swiped = &handler` with `void handler(lv::swipe)`.
2. The gesture fires at LVGL's 50 px while the finger is down.
3. The thresholds are a token group `gesture`, given to `lv::pointer` in its constructor. Signal gets a `gesture` group with LVGL's defaults (`tools/signal_sync.py --record`; nothing is written back to Claude Design). The speed is a plain length for each read, documented with the read period.
4. The hold's progress is an `lv::state<lv::percent>`.
5. A percent that an arc follows through `lv::computed` is enough: no angle state.
6. An explicit way to turn the swipe off for a subtree, which clears the bubble flag and so also stops a waiter above it. Its name and form are proposed in the pull request that builds it.

`lv::dragged` and the finger-following page (gap 5) are out of scope (v0.2).

## Cost

Both would be opt-in (`lv/gesture.hpp`, not in `lv/lv.hpp`): an event source and an animation step, no scan of LVGL's names. The measure is a probe file that awaits a swipe and a hold, next to the other probes; `include_only` must not move.
