# Work that nobody sees: what to pause, and what must go on

Research of 2026-11-19, against `origin/main` at `fcd093f`.
Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-19 unless its row says otherwise.
No library code was written. The probe was compiled outside the tree and thrown away.

The owner's question: when a widget runs an animation and is hidden, nobody sees it, so why process it?
The saving is rendering time. The situation changes when audio is attached: a podcast video need not be visible to be useful.
This note widens the question from "a window is hidden" to "something is not visible", at every level, and asks what must keep going anyway.

It builds on `docs/research/task-handles.md`. Decided there: `lv::run<fn>` pauses while its display is hidden, `lv::run<fn, lv::always>` keeps running, time keeps counting, a task "wakes once, at once", and `lv::display` carries a visible state.

## Short answers

- **LVGL already stops rendering for most hidden things, and the saving is large.**
  A widget hidden by its flag or an ancestor's costs 0.6 to 0.9 M instructions a second.
  So does one scrolled out, clipped, or on a screen that is not active.
  That is for 24 animated arcs. The same arcs visible cost 438 M. `measured`
- **LVGL does not stop the callbacks.** Every `lv_anim` of a hidden widget keeps calling its `exec_cb`, and every timer and observer keeps firing. That is the 0.6 to 1.0 M, about 570 instructions per call. `measured`
- **Two cases LVGL does not detect.**
  A widget covered by an opaque sibling makes the display flush the same pixels, and costs 42.5 M a second.
  A widget at opacity 0 costs 74.9 M and flushes the same pixels. `measured`
- **A hidden window is the big one, and LVGL cannot know it.** LVGL keeps rendering into a frame nobody sees: 438 M a second. Stopping invalidation takes it to 15 M. Hiding the screen object takes it to 1.1 M. `measured`
- **The pattern in the toolkits read:** visibility is a state that the toolkit reports and the program acts on. Drawing stops by itself. Logic stops only by an explicit act or a declared policy. Work with an effect outside the screen, such as audio, is exempt by declaration. `source`
- **Recommendation.** Build the display level only, as decided: a visible state that stops invalidation and pauses `lv::run` tasks.
  Leave widget-level visibility to LVGL. Do not track it, because its callbacks cost 0.1% of the visible cost.
  A task, an animation or a binding whose only effect is pixels may pause. A task with an effect elsewhere says `lv::always`. For the podcast, split the task.
- **Animations keep their clock.** They are not paused separately. On show, they are where they would be now, as a sleeping task wakes at once. `measured` for the cost, `reasoned` for the rule.

## What LVGL does today

### How it was measured

- A probe in plain C against the C API, with no lvgl26. It was built twice, against LVGL v9.6.0 and against the v10 pin, with GCC 16.2 on an arm64 host.
- 24 animated widgets of 70 by 70 pixels on a 480 by 320 display, in partial render mode with a 480 by 40 buffer. A flush callback counts calls and pixels.
- Three kinds of widget: an arc moved by an `lv_anim` with a 1 second period, and a label whose text a timer sets every 100 ms.
  The third is a label bound to a subject that a timer changes every 100 ms.
- Ten and then twenty simulated seconds, in steps of 5 ms. The cost is `/usr/bin/time -l` "instructions retired" of the whole process.
  The cost of a second is the difference between the 20 s run and a 0 s run, divided by 20. It leaves out start-up and the first draw.
- The two pins agree within 0.3% on every row. The table gives v9.6.0, and the text names a difference where there is one. `measured`

### What happens to a widget nobody sees

Arcs, instructions in millions for one simulated second, and what the display flushed.

| Condition | Cost a second | Against visible | `exec_cb` calls a second | Pixels flushed in 20 s |
|---|---|---|---|---|
| Visible | 438.3 M | 100% | 686 | 79.4 M |
| Hidden by its own flag | 0.61 M | 0.14% | 686 | 0 |
| Hidden by its parent's flag | 0.92 M | 0.21% | 686 | 0 |
| Scrolled out of its container | 1.02 M | 0.23% | 686 | 0 |
| Outside its parent's bounds (clipped) | 1.02 M | 0.23% | 686 | 0 |
| On a screen that is not active | 0.84 M | 0.19% | 686 | 0 |
| Covered by an opaque sibling | 42.5 M | 9.7% | 686 | 79.4 M |
| Opacity 0 | 74.9 M | 17.1% | 686 | 79.4 M |
| Hidden, and its animations paused | 0.22 M | 0.05% | 0 | 0 |

- A hidden, scrolled, clipped or off-screen widget is not invalidated, so nothing is rendered or flushed. The `exec_cb` still runs. The anim list and the timers are not told. `measured`
- LVGL's rule is in `lv_obj_area_is_visible`. Invalidation stops there.
  It returns false for a hidden widget, a hidden ancestor, and a screen that is not the active, previous or a layer screen.
  It also returns false for an area outside the widget and an area clipped away by a parent. `source` (`src/core/lv_obj_pos.c`)
- Covered by an opaque sibling: the draw skips what is under the cover, which saves 90% of the instructions. The covered widget still invalidates, so the display flushes the same 4 M pixels a second. On a device that is the SPI or DMA traffic. `measured`
- Opacity 0 is a style. LVGL invalidates and renders the widget, and the pixels are the same as when visible. `measured`
- The idle machinery of 24 anims and the refresh timer, with nothing to do, costs 0.22 M a second. So 24 hidden animations add about 0.39 M a second for 686 calls: about 570 instructions a call. `measured`
- A label whose text a timer sets, 24 widgets at 10 Hz: visible 33.4 M a second, hidden 1.0 to 1.2 M, covered 13.7 M, at opacity 0 22.5 M. A bound label behaves the same, at 33.9 M visible and 1.05 to 1.3 M hidden.
  A hidden widget still gets its text set. 240 `lv_label_set_text` calls a second still run: the text of the widget is right when it is shown. `measured`
- Timers fire whatever is visible. An observer of a subject fires and sets the property of a hidden widget. `measured`

### What LVGL offers to stop the work

| Tool | Where | What the probe showed |
|---|---|---|
| `lv_obj_set_hidden`, the hidden flag | v9.6.0 and v10 | Drops invalidation of the widget and its children at once: 0.61 M a second |
| `lv_display_enable_invalidation(disp, false)` | v9.6.0 and v10 | No widget is invalidated: 15.0 M a second. It is more than a hidden widget because each property setter does its work up to the gate in `lv_inv_area` |
| Hiding the screen object | Same flag on the screen | 1.09 M a second, and one full redraw when it happens (4 flushes, 153,600 pixels) |
| `lv_anim_pause`, `lv_anim_resume`, `lv_anim_pause_for` | v9.6.0 and v10 | `exec_cb` stops: 0 calls. The clock freezes: `act_time` is not advanced while paused, and a resumed animation goes on from where it stopped. `source` (`lv_anim.c`) |
| Pausing the refresh timer of the display | `lv_display_get_refr_timer` | No effect: 438.7 M a second. LVGL pauses that timer after each refresh and resumes it on the next invalidation. `source` (`lv_refr.c`, `lv_display.c`) |
| `lv_timer_pause`, `lv_timer_resume` | v9.6.0 and v10 | Resume does not change the timer's last run: time spent paused still counts |
| `lv_obj_is_visible(obj)` | v9.6.0 and v10 | A pull query, not an event. 115 to 905 instructions a call (below) |
| `LV_EVENT_COVER_CHECK` | v9.6.0 and v10 | Asked at draw time for the opaque cover. It does not stop the flush |
| `LV_EVENT_SCREEN_LOAD_START`, `SCREEN_LOADED`, `SCREEN_UNLOAD_START`, `SCREEN_UNLOADED` | v9.6.0 and v10 | Tell a screen's objects that it became active or not |

- There is no LVGL event for "this widget became hidden or visible". Neither pin has one: the flags and scrolling send none that says so. `source` (`lv_event.h`)
- The cost of `lv_obj_is_visible` in instructions a call: visible 905, hidden by its flag 115, hidden by its parent 555, scrolled 711, clipped 706, on another screen 444. `measured` (v9.6.0 only)
- It answers "visible" for a covered widget and a widget at opacity 0 (905 for both). LVGL's notion is geometric. `measured`
- The v10 pin allocates a subject with `lv_subject_create(type)`, where v9.6.0 initialises one in place with `lv_subject_init_int`. It binds with `lv_obj_bind_int` and a callback, where v9.6.0 has `lv_label_bind_text`. That is the one API difference the probe met outside the hidden flag.

### What a Cortex-M4 would differ in

- The instruction counts come from the same C code, software rendering included, so the ratios carry over. `reasoned`
- The absolute numbers do not: the visible scene costs 438 M a second, which a 150 MHz Cortex-M4 could not run in real time. A device draws less, and the visible side of every ratio shrinks with it. `reasoned`
- The flush side is worse on a device. Covered and opacity-0 widgets flush 4 M pixels a second, which is 8 MB a second at 16 bits and has to cross a bus. `reasoned`
- Nothing here was run on a target. The ARM nightly could carry the probe (see the check below).

## What lvgl26 adds on top

- **`lv::animate`** starts an `lv_anim` and keeps its pointer (`running`) while it runs. A pause is possible with `lv_anim_pause(running)`. It is the same animation as the arcs: its cost when hidden is the 570 instructions a call above. `source` (`include/lv/animate.hpp`)
- **Bindings** follow a state through a `detail::watch_node` and set a property of the widget. They do not look at visibility, so a bound label of a hidden widget gets its text set: the 4,000 instructions a set that the label rows show. `source` (`CLAUDE.md`, `include/lv/detail/watch.hpp`), `measured` for LVGL's side
- **Tasks that sleep and set properties** wake on an LVGL timer and set a property. A hidden widget makes the set cheap, but the wake and the coroutine step are paid. `source`
- **`lv::each` rows off screen.** `docs/research/lists.md` plans recycling of rows. A row that is not built costs nothing, which is the better answer than a hidden one. Not measured here.
- **A page of `lv::shown`** does not exist while it is not shown: its tree is not built, its storage is not made, and a `lv::run` in it has ended. It comes back when the page is shown, and starts again. `source` (`CLAUDE.md`, `one_of_region`)
  That is "ended and restarted", not "paused and resumed", so the state of such a task must live in an `lv::state`.

## What others do

All pages were read through a fetch tool unless a row says otherwise. Marks: `source` is a primary page read, `summary` is a secondary one.

| System | What stops when it is not visible | What goes on | Who decides |
|---|---|---|---|
| Browser, `requestAnimationFrame` | "paused in most browsers when running in background tabs or hidden iframes" | Timers, throttled | The browser |
| Browser timers | Throttled to once a second in a background tab; Chrome 88 checks a hidden tab's chained timers once a minute after 5 minutes | "Tabs which are playing audio are considered foreground and aren't throttled"; WebSocket, WebRTC, IndexedDB are exempt | The browser, with exemptions for live effects |
| `IntersectionObserver` | Nothing; it reports | With `trackVisibility: true` it also reports "hasn't been covered by other elements", conservatively and with a delay of at least 100 ms | The page |
| CSS `content-visibility: auto` | Rendering work of off-screen content is skipped "until it is needed"; the event `contentvisibilityautostatechange` says when | Layout size is kept | The page, by declaring it |
| CSS `display: none` | "terminate any running animation applied to the element and its descendants"; it starts again on return | | The browser |
| CSS `animation-play-state: paused` | "Resuming a paused animation will start the animation from where it left off" | | The page |
| Browser media | Audible tabs count as foreground. Media Session gives controls "even when not looking at the web page". Whether video decoding stops when hidden: not confirmed | Audio | The page, through Media Session |
| Android, Lifecycle | Flow collection "begins when the lifecycle is `STARTED` and stops when the lifecycle is `STOPPED`" | A `LaunchedEffect` "can run even if the composable is not currently visible"; a foreground service and audio focus keep playback | The program, by scope |
| Jetpack Compose lists | Lazy lists "only compose and lay out items which are visible" | | The toolkit |
| SwiftUI | `scenePhase` inactive: "should pause its work". `AnimationTimelineSchedule` is "pausable" | The playback audio category with the background audio mode keeps sound | The program |
| Flutter | `TickerMode` "Enables or disables tickers (and thus animation controllers) in the widget subtree" | `Offstage`: "Animations continue to run in offstage children". `Visibility.maintainAnimation` says to keep them | The program, with a subtree switch |
| Qt Quick | A hidden window "will stop rendering"; `visible` is what the application wants and `isExposed` what the system gives. `Loader.active` false releases the item | Whether animations of an invisible item run: not confirmed. Qt Multimedia: not confirmed | The program |
| Slint | `visible: false` stops drawing and mouse input of an element and its children | The element keeps its layout space. A `Timer` runs whatever is visible | The program |
| Unity | `Application.runInBackground` false: "the application pauses when it is in the background" | `OnApplicationPause` tells the objects | The engine, by default |
| SDL3 | Reports: "Window has been hidden", "minimized", "occluded", "about to enter the background" | | The program |
| Game loops | "the renderer produces time and the simulation consumes it in discrete dt sized steps" (Gaffer On Games) | | The program |

Sources:

- The browser rows: MDN pages on [`requestAnimationFrame`](https://developer.mozilla.org/en-US/docs/Web/API/Window/requestAnimationFrame), the [Page Visibility API](https://developer.mozilla.org/en-US/docs/Web/API/Page_Visibility_API), [Intersection Observer](https://developer.mozilla.org/en-US/docs/Web/API/Intersection_Observer_API), [`content-visibility`](https://developer.mozilla.org/en-US/docs/Web/CSS/content-visibility), [`animation-play-state`](https://developer.mozilla.org/en-US/docs/Web/CSS/animation-play-state), the [Media Session API](https://developer.mozilla.org/en-US/docs/Web/API/Media_Session_API); the [CSS Animations spec](https://www.w3.org/TR/css-animations-1/); [Chrome 88 timer throttling](https://developer.chrome.com/blog/timer-throttling-in-chrome-88). `source`
- Android: the pages on [coroutines with architecture components](https://developer.android.com/topic/libraries/architecture/coroutines), [side effects](https://developer.android.com/develop/ui/compose/side-effects), [lists](https://developer.android.com/develop/ui/compose/lists) and [foreground services](https://developer.android.com/develop/background-work/services/fgs). `source`
- Apple: the pages on `ScenePhase`, `AnimationTimelineSchedule`, `TimelineView` and `AVAudioSession`. `source`
- Flutter: [`TickerMode`](https://api.flutter.dev/flutter/widgets/TickerMode-class.html), `Offstage`, `Visibility`, `AppLifecycleState` and `ListView.builder` at api.flutter.dev. `source`
- Qt: [`QQuickWindow`](https://doc.qt.io/qt-6/qquickwindow.html), [`QWindow`](https://doc.qt.io/qt-6/qwindow.html), [`Item`](https://doc.qt.io/qt-6/qml-qtquick-item.html), [`Animation`](https://doc.qt.io/qt-6/qml-qtquick-animation.html), [`Loader`](https://doc.qt.io/qt-6/qml-qtquick-loader.html). `source`
- Slint: the reference pages for [`Timer`](https://docs.slint.dev/latest/docs/slint/reference/timer/) and the [common element properties](https://docs.slint.dev/latest/docs/slint/reference/common/). `source`
- Unity: `Application.runInBackground` and `MonoBehaviour.OnApplicationPause`. SDL3: the [event list](https://wiki.libsdl.org/SDL3/SDL_EventType). Gaffer: [Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/). `source`
- Not read: LVGL's documentation site, which did not serve. The headers and sources of the two pins were read instead.
- Not read: TouchGFX, Embedded Wizard, Crank, Core Animation off screen, `ValueAnimator` on an invisible view, `View.onVisibilityChanged` and `withFrameNanos`.
- Not read: whether a browser decodes a hidden video.

### The common pattern

1. **Visibility is a state that the toolkit reports.** The program, or a subtree, decides what to do with it. Browsers report it with an event, Qt as `visible` against `isExposed`, SDL as events, Flutter as a `TickerMode` that flows down a subtree.
2. **What produces pixels stops, by itself.** `requestAnimationFrame`, a hidden window's rendering, `display: none` animations, lazy lists, a hidden Slint element's drawing.
3. **Logic stops only by an explicit act or a declared policy.** `Loader.active`, `Timer.running`, `repeatOnLifecycle`, `runInBackground`, `TickerMode`. Slint's timers and Flutter's `Offstage` animations go on.
4. **Work with an effect elsewhere is exempt by declaration.** Audible tabs, WebRTC, foreground services, audio focus, the background audio mode.

### Where they disagree

- **The default.** Unity and Apple's `inactive` say pause; Flutter's `Offstage`, Slint's timers and Compose effects say go on; browsers throttle but do not cancel.
- **The unit.** Flutter switches a subtree (`TickerMode`). Browsers decide a tab. Qt decides a window. Android decides a lifecycle scope.
- **What "visible" means.** A browser may report "visible" for a window that is fully covered, and its `IntersectionObserver` can say "covered" only conservatively. LVGL's `lv_obj_is_visible` also ignores a cover.

The closest design to the owner's question is Flutter's `TickerMode`: a switch that flows down a subtree and turns animations off, with a way to keep some.
The model below uses the same shape at the unit lvgl26 already owns, the display and the mounted tree.

## The model for lvgl26

### Levels of "not visible"

| Level | LVGL knows it | How lvgl26 would learn it | Cost of learning | What pauses |
|---|---|---|---|---|
| 1. The display or window is hidden | No: it keeps rendering | The platform writes `display.set_visible(false)` (decided) | An event, no cost between changes | Everything below, for every tree of the display |
| 2. The screen is not the active one | Yes: `SCREEN_LOADED` and `SCREEN_UNLOADED`; no invalidation | A callback on the screen events, once for each display | One LVGL event descriptor for each display | Drawing already; tasks and bindings of its trees if lvgl26 follows it (below) |
| 3. A page of `lv::shown` that is not the current arm | Not applicable: it is not built | It does not exist, so nothing to learn | None | Nothing to pause: its tasks ended and start again when it is shown |
| 4. A widget hidden by its flag or an ancestor's | The flag, with no event | Through lvgl26's own setters only; a flag set through the C API is unseen. Or a pull with `lv_obj_is_visible` | 115 to 555 instructions a pull | Drawing already |
| 5. A widget outside the visible area (scrolled, clipped) | Geometry, with no event but `SCROLL` | A pull only | 706 to 711 instructions a pull | Drawing already |
| 6. A widget fully covered or at opacity 0 | No: it renders and flushes | A program that knows what it covers | None | Nothing, unless the program says so |

What the measurements say about the levels:

- **Levels 4 and 5 are already cheap.** LVGL stops rendering.
  The callbacks that remain cost 0.4 to 1 M instructions a second for 24 widgets, which is 0.1% to 0.2% of the visible cost.
  Tracking them would cost more than it saves.
- **Level 1 is the large one, and only the platform knows it.** 438 M a second against 1 to 15 M.
- **Level 6 is the real waste after level 1.** It costs 10% to 17% of the visible cost and 100% of the flush. LVGL has no event for it, and neither has a browser.
- **Level 2 is nearly level 1.** A screen that is not active already costs 0.84 M. What lvgl26 can add is pausing the tasks of its trees, which would be useful for a program of several screens.

### What pauses

| What | Pauses when | How | Saved |
|---|---|---|---|
| Drawing | Levels 2 to 5: by LVGL. Level 1: by lvgl26 | `lv_display_enable_invalidation(disp, false)` while the display is hidden, then an invalidation of the screen when it is shown | 96.6% of the visible cost for arcs; 99.8% if the screen object is hidden instead |
| `lv::run<fn>` tasks | The display is hidden (decided) | The gate in `resume_task`, "wakes once, at once" | The coroutine steps |
| `lv::run<fn, lv::always>` | Never | | |
| `lv::animate` of a paused task | With its task | An animation follows its task, see below | |
| Free `lv::animate`, not awaited by a task | Nothing, in the first build | They keep their clock; invalidation is off, so nothing is drawn | The rest is 0.39 M a second for 24 animations. Not worth the machinery |
| Bindings of a tree on a hidden display | The tree's display is hidden | The binding sets nothing and marks itself dirty. On show it applies the state's latest value | 4,000 instructions a text set. Needs a dirty bit and a walk on show |
| Tasks under a `lv::run` deeper than the root | The display is hidden. Not the node's own parent | The gate follows the display only | See the question below |
| List rows off screen | Not built, by the list design | | All of it |

**A `lv::run` on a sub-tree.** Does it pause with that sub-tree being hidden?

- Option A: only with the display. One flag, event driven, nothing to learn at levels 4 and 5.
- Option B: with its parent widget, by a pull at each wake-up: `lv_obj_is_visible(parent)` costs 115 to 905 instructions on every wake. A held wake-up then needs something to resume it, and there is no LVGL event for "became visible". A poll timer would do, at 36 bytes and a wake for each held task.
- Option C: B with a draw event on the widget to resume. It needs an LVGL event descriptor for each task.

**Recommendation: A.** A task that wants to skip work for a hidden widget asks itself: `lv::is_visible(view)` is a thin call of `lv_obj_is_visible`.
That is the pattern of Android and Flutter: the toolkit reports, the program decides. Cost: one function, no machinery.

### What must keep going, and how the program says so

The rule: **work whose only effect is pixels may pause. Work with an effect outside the screen does not.**

- An effect outside the screen is sound, a network transfer, a motor, a measurement, a timer the user set, a log.
- A task is the unit of program logic, so the program says it: `lv::always` on the task (decided).
- An animation of a widget property and a binding to a widget property have no effect but pixels, by what they are. They need no `always`.
- An animation awaited by an `lv::always` task is part of that task. It keeps running, because the task needs it to end. It draws nothing while invisible.
- So the rule "split the task: one for the sound with `lv::always`, one for the picture without" is enough, and the state is what joins them.

The decided rule for tasks makes the split natural: logic writes an `lv::state`, and the pixels follow the state. A hidden tree's bindings catch up when it is shown, at the latest value.

### The podcast video, under this proposal

The sound must go on when the window is hidden. The picture and the progress ring need not.

```cpp
constinit lv::state progress{lv::counter{}};    // seconds played
constinit lv::state title{std::string_view{"Episode 12"}};

// Logic with an effect outside the screen: it plays the sound and keeps the position.
lv::task<> play_audio() {
  for (;;) {
    co_await lv::sleep(1000 * lv::ms);
    ++progress;                       // the pixels follow this state
  }
}

// Pixels only: it moves the artwork. It pauses with the display.
lv::task<> drift_artwork(lv::view<"artwork"> artwork) {
  for (;;) {
    co_await lv::animate(artwork, {.x = {0 * lv::px, 12 * lv::px}}, 4000 * lv::ms);
    co_await lv::animate(artwork, {.x = {12 * lv::px, 0 * lv::px}}, 4000 * lv::ms);
  }
}

constexpr auto player = lv::column({...})[
    lv::image({.id = "artwork", .src = cover}),
    lv::label({.text = lv::bind(title)}),
    lv::arc({.value = lv::bind(progress)}),
    lv::run<play_audio, lv::always>,          // keeps running with the window hidden
    lv::run<drift_artwork, "artwork">];       // pauses with it
```

- While the window is hidden: `play_audio` goes on and sets `progress` each second. Nothing is drawn, since invalidation is off. The arc's binding marks itself dirty and sets nothing.
- `drift_artwork` is held at its `co_await`. When the window is shown it resumes once, at once, and the artwork is where the animation would be now.
- The arc then shows the latest `progress`, not the first change that happened while hidden.
- A real player would call the platform's audio from `play_audio`. That call is the effect that makes the task `always`.
- The task could ask for the window itself: `lv::display_of(artwork).show()` on a notification, which the decided design allows.

### Resuming an animation

When the display is shown again, what should an animation do?

- **Option 1: jump to where it would be now.** It keeps its clock while hidden, and nothing is drawn. This is what the decided rule gives a task that awaits it: time keeps counting, the wait ends while hidden, the task wakes once at once.
  Cost: the 0.39 M a second for 24 animations, and nothing else. No LVGL pointer is kept.
- **Option 2: freeze, and continue from where it stopped.** `lv_anim_pause` does this. It stops the callbacks too: 0.22 M a second instead of 0.61 M.
  Cost: the pointer of each running animation, a pause and a resume call for each, and a rule that differs from a sleeping task's. A transition frozen after one of its two seconds would delay its task by the hidden time.
  A `lv::sleep` does not.
- **Option 3: restart.** The animation begins again on show. It loses the position and cannot be awaited consistently.

**Recommendation: option 1.** It follows the clock rule that the owner decided for tasks. The saving of option 2 is 0.39 M a second for 24 animations, which is 15 thousand instructions a second for each. Option 2 stays open for an infinite decorative animation, as `lv::animate(..., lv::paused_when_hidden)` if a use appears.

### Defaults and the opt-out

- Pausing invisible work is on by default. The owner decided it for `lv::run`, and it saves the most where nothing is lost.
- A project may switch it off. `docs/research/settings.md` was not on `main` or on any branch of the remote on 2026-11-19, so this follows `CLAUDE.md`: "Settings go in `lv::config`".
  The proposal is one field, `pause_when_hidden`, default true. With it off, `lv::run<fn>` means `lv::run<fn, lv::always>`, and a hidden display keeps its invalidation. If the settings design puts switches in `lvgl26.json`, the key is the same.
- A per-task opt-out already exists: `lv::always`. There is no per-display opt-out. Add it only for a use.

### A check that it works

A hidden widget costs nothing only if a test says so, or the saving will regress.

- **A ctest test with counters.** Hide a display whose panel counts the pixels of every `area` it is told. Run animations, a task, a bound label and a sleeping loop for 5 simulated seconds. Assert that no pixels are flushed after the hide.
  Assert that the loop's counter did not move and the `always` task's did.
  Assert that the label's text is the latest on show.
  It needs no instruction counter, so it runs under every preset. `reasoned`
- **A run-time probe for the nightly metrics job.** The probe of this note is 150 lines. It prints one row for each scenario: `exec_cb`, flushes, pixels and instructions a second.
  The job being designed for compile time can carry it, with a threshold on the ratio of hidden to visible: today 0.2% for a hidden widget and 3.4% for invalidation off. The ratio is stable to 0.3%, so a regression of 10% in the hidden cost shows. `reasoned`
- The same probe on the Cortex-M4 nightly would give the numbers this note could not. `reasoned`

## Options for the owner

### 1. The unit of visibility

| Option | What it is | Cost |
|---|---|---|
| A. The display | One visible state, decided. Trees follow it | An event on the platform change. Nothing a widget pays |
| B. The display and a tree | A tree may also be hidden by the program: `tree.hide()` hides its root widget and pauses its `lv::run`, its animations and its bindings. It is Flutter's `TickerMode` at the tree. It is how a page covered by a modal would stop | A bool and a watch list for each tree, and the walk on show |
| C. Every widget | Visibility derived for each widget and followed | No LVGL event exists. A pull costs 115 to 905 instructions and needs a poll timer to resume |

**Recommendation: A now.** B when a covered page appears, and A already gives the hook. C: no.
Level 6 costs 10% to 17% of the visible cost and all the flush.

### 2. How a hidden display stops drawing

| Option | Cost a second, 24 arcs | What it touches |
|---|---|---|
| Invalidation off | 15.0 M | Nothing of a widget |
| Hide the screen object | 1.09 M, and one redraw when hiding | The screen's flag, which the program may also set |
| Invalidation off and lvgl26's animations paused | 0.24 M | The pointers of running animations |

**Recommendation: invalidation off.** One call, and it touches no widget state. If the tail of 15 M proves too large in the nightly numbers, hide the screen object instead. It is 14 times cheaper for animations that lvgl26 does not own.

### 3. Pausing animations

**Recommendation: do not pause animations by themselves** (option 1 above). They follow the clock, and their paused task stops with the display.

### 4. Bindings of a hidden tree

**Recommendation: skip and mark dirty, apply the latest value on show.** It is the rule the task design already gives for state changes. Cost: a dirty bit in each tree's storage and a walk of its bound nodes on show.
Measure it before building: the 4,000 instructions a set are small beside the walk if few bindings change. `reasoned`

## Not confirmed

- Nothing here ran on a Cortex-M4 or with lvgl26. The probe used LVGL's C API. The numbers for `lv::animate`, bindings and tasks are reasoned from it.
- Why `lv_display_enable_invalidation(false)` leaves 15 M a second for arcs while hidden labels cost 1.0 M. The probe did not look into `lv_arc_set_value`.
- `lv_obj_is_visible` was measured on v9.6.0 only.
- The first full redraw on show was not timed. LVGL redraws what was invalidated, and an invalidation of the whole screen on show would cost one full frame.
- Whether a platform window hidden by minimize or by occlusion should count as hidden. SDL has `hidden`, `minimized` and `occluded` events, and the adapter of pull request #132 is not read here.
- Qt: whether animations of an invisible item run, and whether Qt Multimedia plays on with its video output hidden. Core Animation off screen. A browser's video decoding when hidden. Not confirmed by any page read.
- `docs/research/settings.md` was not found, so the opt-out follows `CLAUDE.md`.
- The cost of an LVGL event descriptor for the screen events of level 2 was not measured.

## Decisions for the owner

1. **Unit of visibility.**
   Options: the display; the display and a tree; every widget.
   Recommendation: the display now, a tree when a covered page appears, never every widget.
   Cost: see the table above.
2. **How a hidden display stops drawing.**
   Options: invalidation off; the screen object hidden; invalidation off with animations paused.
   Recommendation: invalidation off, and hide the screen object if the tail matters.
   Cost: 15 M against 1.09 M a second for 24 arcs.
3. **Animations.**
   Options: keep the clock; freeze; restart.
   Recommendation: keep the clock, as for a sleeping task.
   Cost: 0.39 M a second for 24 animations.
4. **Bindings of a hidden tree.**
   Options: set always; skip and apply the latest on show.
   Recommendation: skip and apply the latest.
   Cost: a dirty bit and a walk on show.
5. **A `lv::run` under a hidden widget.**
   Options: with the display only; with its parent by a pull; with its parent by a draw event.
   Recommendation: with the display only, and `lv::is_visible(view)` for a task that wants to ask.
   Cost: one thin function.
6. **Screens.**
   Options: leave a screen that is not active to LVGL; pause the tasks of its trees too.
   Recommendation: leave it to LVGL now. Revisit with a program of several screens.
   Cost of the second: an LVGL event descriptor for each display.
7. **The opt-out.**
   Options: a switch for all pausing; none.
   Recommendation: one field, `pause_when_hidden`, default true.
   Cost: a branch in the node and the display.
8. **The check.**
   Options: a ctest test with counters; the probe in the nightly job; both.
   Recommendation: both. The test guards the logic and the probe guards the saving.
   Cost: the test is small, and the probe is 150 lines already written.
