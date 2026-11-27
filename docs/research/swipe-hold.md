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
- The event goes to the pressed widget, or to its first ancestor that does not have `LV_OBJ_FLAG_GESTURE_BUBBLE` (every widget has it by default, so the event reaches the screen unless something clears the flag). A handler on a page's root therefore sees a swipe that starts anywhere on the page. `source`, `src/core/lv_obj.c`.
- The defaults, per input device: `gesture_min_distance` 50 px, `gesture_min_velocity` 3 px for each read (a read comes every 33 ms, so about 90 px/s), settable with `lv_indev_set_gesture_min_distance` and `lv_indev_set_gesture_min_velocity`, both 8 bits. `source`: `lv_conf_template.h`, `src/indev/lv_indev.c`.
- No gesture is sent while a scroll is running (`if (indev->pointer.scroll_obj) return;`). So a drag that LVGL takes as a scroll never becomes a swipe. `source`.

## How LVGL decides that a drag scrolls

- After the pointer moved `scroll_limit` pixels (10 by default) from the press, `lv_indev_find_scroll_obj` looks for an object to scroll: the pressed widget and its ancestors, the first that can scroll in the direction of the drag (it has content beyond its edge on that axis). If none can scroll in that direction but one can scroll on that axis, it is the candidate and shows the elastic effect. If none can scroll on that axis, there is no scroll object and the drag stays a candidate for a gesture. `source`: `src/indev/lv_indev_scroll.c`.
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

## Questions for the owner

1. Is the swipe one wait that gives a direction (A), with the filter by template arguments? Or one name for each direction (B)?
2. The gesture is sent while the finger is still down, at 50 px, not on release. Is that right for back, next and Service? (The finger-following page of gap 5 needs the drag itself, not this event; it is a separate `lv::dragged` and is not part of this note.)
3. Are the thresholds a `gesture` group of tokens, given to `lv::pointer` in its constructor, with LVGL's defaults in Signal? The speed token is awkward: LVGL measures pixels for each read, and a read is 33 ms. Is a plain length for each read acceptable, or do you want pixels for each second converted at the display's read period?
4. The hold's progress: a state of `lv::percent` (my recommendation), or a new `lv::progress`?
5. Should the hold also accept a progress `lv::state` of an arc's angle, or is a percent that an arc follows through a `lv::computed` enough?
6. Brewing "swipes off": is the absence of a waiter on that page enough, or do you want an explicit way to turn the swipe off for a subtree (clearing the bubble flag), which also stops a shell-level waiter?

## Cost

Both would be opt-in (`lv/gesture.hpp`, not in `lv/lv.hpp`): an event source and an animation step, no scan of LVGL's names. The measure is a probe file that awaits a swipe and a hold, next to the other probes; `include_only` must not move.
