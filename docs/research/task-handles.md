# A handle that controls the lifetime of a spawned task

Design note of 2026-11-19, against `origin/main` at `402fa76`.
Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-19 unless its row says otherwise.
No product code was written. One probe was compiled outside the tree and thrown away.

The question from the owner, about `lv::spawn(demo::tick())` in `examples/wasm/counter/app.hpp`: can a program hold a running task and end it?
Today a spawned task ends in one of three ways.
It returns, or a widget it has waited on is deleted, or the owner given to `lv::spawn(task, owner)` is deleted.
`lv::spawn` returns a `bool`, and nothing else reaches the task. `source` (`include/lv/task.hpp`)

## Short answers

- **The example that started this has a bug a handle would fix.** `demo::tick()` loops for ever and has no owner.
  When `app::running` is destroyed its tree goes, and the task goes on ticking. `source` (`examples/wasm/counter/app.hpp`, lines 50 to 68)
- **Others split on one question: what does dropping the handle do?** Rust's `JoinHandle`, Swift's `Task` and Kotlin's `Job` leave the task running.
  `std::jthread`, tokio's `JoinSet`, Slint's `Timer` and Trio's nursery end the work. `source`
- **None of the systems read lets a cancelled task await a cleanup.** Cleanup is a destructor, or an opt-out (`NonCancellable`, shielding). `source`
- **Recommendation: keep `lv::spawn` as it is, and add `lv::start`, which returns a move-only owning `lv::job`.**
  Dropping the job ends the task. `[[nodiscard]]` stops an accidental drop. `job.detach()` is the explicit way to let go.
- **The handle needs no allocation.** A back-link costs 8 bytes in the frame and 8 in the handle.
  The handle clears itself when the task ends. `measured`
- **The best home for the example is a tree node, not a handle in a variable.**
  `lv::run<demo::tick>` would start the task when the tree is mounted.
  It would end the task with the tree. `reasoned`
- **Not recommended now:** a result value from a detached task, pause and resume, and a parent that cancels its children by itself.

## What others do

Each row answers four questions: who owns the task, what dropping the handle does, how cancellation arrives, and what the waiting side sees.

| System | Owner | Dropping the handle | Cancellation | The waiter sees |
|---|---|---|---|---|
| `std::jthread` | The thread object, move-only | `request_stop()`, then `join()` | A `stop_token` passed to the function; the function polls or registers a `stop_callback` | `join()` returns; no result |
| `std::execution::spawn` with a scope (C++26 draft) | A `counting_scope`, not the caller | No handle: `spawn` returns `void` | `scope.request_stop()` on a `counting_scope`; `simple_counting_scope` has no stop | Nothing; the work must finish with `set_value()` or stopped. A scope destroyed with work outstanding calls `terminate` |
| `spawn_future` | The scope tracks it; the sender is the handle | A sender dropped unconnected requests stop and loses the result | Internal stop source | The result, when the sender is connected |
| `std::execution::task` (P3552) | The coroutine handle until `connect`, then the operation state | An unstarted task destroys its frame; the body never runs | The stop token of the receiver's environment; a `co_await` on a stopped token ends the coroutine at once | `set_stopped`. "there is currently no plan to support asynchronous clean-up" |
| libunifex `async_scope` | The scope | `spawn()` returns a future; discarding it requests cancellation | `cleanup()` requests stop and waits | A scope destroyed before `cleanup()` completes asserts |
| cppcoro | `task<T>` is move-only and lazy; `async_scope::spawn` starts eagerly | A task dropped unawaited never runs | A `cancellation_token` passed as a parameter and polled | The result, or `terminate` for an exception in spawned work |
| Trio | The nursery | No handle: `start_soon` returns nothing | `Cancelled` raised at the next checkpoint; a shield opts out | The nursery block ends after all children finished |
| Kotlin `Job` | The parent job | The task keeps running (inferred; no page says it) | `job.cancel()`; the coroutine sees `CancellationException` at a suspension; `NonCancellable` allows cleanup | `job.join()`; `invokeOnCompletion` |
| Swift `Task` | None for `Task { }`; a group owns its children | The task runs to completion | `cancel()` is "cooperative and synchronous": the task polls or checks | `await task.value`; a group awaits its children |
| tokio `JoinHandle` | The runtime | "A JoinHandle detaches the associated task when it is dropped" | `abort()`; the task is dropped at its next await | `JoinError` with `is_cancelled()` |
| tokio `JoinSet` | The set | "all tasks in the JoinSet are immediately aborted" | The same | `join_next()` |
| Embassy | A static pool of `pool_size` slots | Not confirmed | Not confirmed | `SpawnError::Busy` when the pool is full |
| Qt `QFuture` | The computation | "neither waits nor cancels the asynchronous computation" | `cancel()`, asynchronous; a `QPromise` polls `isCanceled()` | `isCanceled()`; a dropped promise cancels itself |
| Qt `QTimer::singleShot` with a context | The context `QObject` | Not applicable | The call is skipped if the context was destroyed | Nothing |
| Slint `Timer` | The timer object | "The timer will automatically stop when dropped" | Drop it | Nothing |
| Slint `spawn_local` | Not stated | Not stated | `abort()` on the returned `JoinHandle` drops the task | The handle implements `Future` |
| Android `viewModelScope` | The view model | Not applicable | Cancelled "if the ViewModel is cleared" | Nothing |
| LVGL | The caller | `lv_timer_t` stays until `lv_timer_delete` | `lv_timer_delete`, `lv_timer_pause`, `lv_anim_delete(var, exec_cb)`; an object delete removes its animations | Callbacks `completed_cb`, `deleted_cb` |

Sources, all read through a fetch tool and so marked `source` unless the table says otherwise:

- jthread: [cppreference jthread](https://en.cppreference.com/w/cpp/thread/jthread), [`~jthread`](https://en.cppreference.com/w/cpp/thread/jthread/~jthread), [stop_callback](https://en.cppreference.com/w/cpp/thread/stop_callback).
- Scopes: [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) and the draft pages [exec.scope](https://eel.is/c++draft/exec.scope), [exec.spawn](https://eel.is/c++draft/exec.spawn), [exec.spawn.future](https://eel.is/c++draft/exec.spawn.future), [exec.associate](https://eel.is/c++draft/exec.associate).
- `std::execution::task`: [P3552R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3552r3.html) and [exec.task](https://eel.is/c++draft/exec.task).
- libunifex: [api_reference.md](https://raw.githubusercontent.com/facebookexperimental/libunifex/main/doc/api_reference.md). cppcoro: [README](https://github.com/lewissbaker/cppcoro/blob/master/README.md).
- Trio: [Notes on structured concurrency](https://vorpus.org/blog/notes-on-structured-concurrency-or-go-statement-considered-harmful/) and the [Trio reference](https://trio.readthedocs.io/en/stable/reference-core.html).
- Kotlin: [launch](https://kotlinlang.org/api/kotlinx.coroutines/kotlinx-coroutines-core/kotlinx.coroutines/launch.html), [NonCancellable](https://kotlinlang.org/api/kotlinx.coroutines/kotlinx-coroutines-core/kotlinx.coroutines/-non-cancellable/).
- Swift: SE-0304 and the Swift book chapter on concurrency, read from their GitHub sources.
- tokio: [JoinHandle](https://docs.rs/tokio/latest/tokio/task/struct.JoinHandle.html), [JoinError](https://docs.rs/tokio/latest/tokio/task/struct.JoinError.html).
- Embassy: [the book](https://embassy.dev/book/), the `embassy-executor` documentation on docs.embassy.dev (`SpawnError`, `TaskStorage`).
- Qt: [QFuture](https://doc.qt.io/qt-6/qfuture.html), [QPromise](https://doc.qt.io/qt-6/qpromise.html), [QTimer](https://doc.qt.io/qt-6/qtimer.html), [QCoro Task](https://raw.githubusercontent.com/qcoro/qcoro/main/docs/reference/coro/task.md).
- Slint: [Timer](https://docs.rs/slint/latest/slint/struct.Timer.html), [spawn_local](https://docs.slint.dev/latest/docs/rust/slint/fn.spawn_local).
- Android: [coroutines with architecture components](https://developer.android.com/topic/libraries/architecture/coroutines).
- LVGL: the headers `lv_timer.h` and `lv_anim.h` of v9.6.0 and the v10 pin, where `lv_timer_pause`, `lv_timer_resume`, `lv_timer_delete` and `lv_anim_delete(var, exec_cb)` exist. `source`

What it means for a single-threaded, allocation-averse UI library:

- Nothing here needs threads. The stop token of `jthread` and `std::execution` is a thread-safe object, and a UI loop needs only a pointer.
- Cleanup that awaits is the part nobody solves cheaply. P3552 says it has no plan, and Kotlin and Trio need an explicit opt-out. lvgl26 can promise less: destructors run, nothing awaits.
- Systems with an owner object match a UI: Trio, Kotlin scopes, Qt parents, Slint `Timer` and `viewModelScope`.
  A screen has a life, and its work should end with it. lvgl26 has that already in `lv::spawn(task, owner)`.
- Embassy is the only allocation-free system read, and its answer is a fixed pool. That matches the decided task region `lv::tasks<>`. `source`

## Where lvgl26 stands today

- A task frame is allocated with `lv_malloc`, from `operator new` of the promise. If it fails, `lv::spawn` returns false. `source`
- A spawned task is `detached`: its frame frees itself at the end. `source`
- A task is tied to every widget it has waited on. A deletion ends it, even while its code runs, by a request that `resume_task` carries out. `source`
- Measured sizes: a task that sleeps takes a frame and a 36-byte timer, 128 bytes in all on the target, and 216 bytes on the host. `measured` (`memory-region.md` and the probe below)
- The decided memory layout has a task region `lv::tasks<Count, Slot>`. A full region makes `lv::spawn` return false. `source` (`memory-region.md`, decisions 4 and 7)
- `lv::held` is a handle that never owns and checks at each use that the slot holds its tree. `lv::mounted` owns, never moves and carries its own failure. `source` (`node-storage.md`, decisions 14 to 23)

## Design questions

### 1. What `lv::spawn` returns

**Option A: a non-owning handle.** `lv::spawn` returns `lv::job`; dropping it leaves the task running.
It is the model of Rust, Swift and Kotlin.

```cpp
lv::job tick = lv::spawn(demo::tick());   // today: bool
if (tick) ...                              // running
tick.cancel();
```

It changes the return type of every call of `lv::spawn`, and `if (!lv::spawn(...))` keeps working only through `explicit operator bool`.
Forgetting the handle is harmless, and so is not using it. The task still outlives its creator, which is the bug of the example.

**Option B: an owning handle.** Dropping the job ends the task, as `std::jthread` and `lv::mounted` do.

```cpp
[[nodiscard]] lv::job tick = lv::start(demo::tick());   // ends with `tick`
```

A dropped result ends the task at once, so `lv::start` is `[[nodiscard]]`. The compiler then names the mistake.

**Option C: both, under two names.** `lv::spawn` stays detached and returns `bool`. `lv::start` returns the owning `lv::job`. `job.detach()` turns one into the other.

```cpp
lv::spawn(blink(lamp), panel);                  // as today: ends with the panel
auto tick = lv::start(demo::tick());            // ends with `tick`, or `tick.cancel()`
auto keep = lv::start(demo::tick()); keep.detach();   // the same as lv::spawn
```

**Recommendation: C, with an owning job.**
Reasons: an owner object is the idiom of this library (`lv::mounted`, `lv::owned`) and of RAII in C++ (`jthread`).
It is also the idiom of the toolkits whose lifetimes the UI shares, such as Qt parents and Slint `Timer`.
`lv::spawn` calls need no change, and its tests stay as they are.
A job held in a coroutine's frame gives structured concurrency without a new mechanism: when the parent ends, its locals are destroyed and the job ends the child (see 6).
Cost: a second name to teach. The owning default makes `lv::start` unsafe to write as a statement, and `[[nodiscard]]` turns that into a warning under `-Werror`.

### 2. How the handle stays valid

The task's frame is freed when the task ends, and the handle may outlive it. Three ways.

| Way | Frame | Handle | Allocates | Copyable | Works with the task region |
|---|---|---|---|---|---|
| Back-link: the promise points at the handle, and clears the handle's pointer in its destructor | +8 bytes | 8 bytes | No | No, move only; a move re-links | Yes: a region slot's destructor clears it too |
| Slot and generation: a handle is a slot number and a generation | 0 | 4 + 4 bytes | No, but a table of generations, 4 bytes a slot | Yes | Only with the region; lvgl26 has no slot table while frames come from `lv_malloc` |
| Control block, as `shared_ptr` | +8 | 8 | Yes, 16 bytes on LVGL's heap | Yes | No: it allocates |

**Recommendation: the back-link.** It is the only one that works today and with the later region, and it allocates nothing.
`lv::weak` is the nearest existing thing, but it asks LVGL for a delete descriptor, which allocates. `source` (`include/lv/owned.hpp`)
Slot and generation is the alternative for the day the region exists and a copyable handle is wanted.

### 3. Cancellation

Today a deleted widget cancels at the next suspension point: the frame is destroyed, and the destructors of its locals run. Nothing is thrown. `source`

- **Option: as today.** A task cannot react but with destructors. `job.cancel()` does what a widget deletion does. It is idempotent, and safe from inside the task itself and from a destructor, because `cancel` already records a request while the code runs. `source` (`cancel`, `resume_state` in `task.hpp`)
- **Option: `lv::cancelled` as the result of each `co_await`.** Every wait would return a status that the task must check. It costs a branch at every await, and a task that forgets to check runs on.
- **Option: a cleanup task.** The handle takes a second task that runs after the first is destroyed. It is an allocation and a second frame, for what a destructor does.

**Recommendation: as today.** A task that must react to the end of a widget waits for it (`view.next().gone`), which exists. A task that must release something owns it in a local with a destructor.
Cleanup that awaits is not offered, as in P3552 and unlike Kotlin and Trio, which need an opt-out.
Cost: none. A task that must wait inside its cleanup starts a new task from the destructor, which is allowed and is a separate decision.

### 4. Waiting for a task, and its result

- `job.running()` and `if (job)`: yes, from the back-link.
- `co_await job.done()`: a wait source like `sleep`, finished when the task ends, also a possible item of `lv::first`. It reuses the field `continuation` of the promise. The awaiter in the waiting frame clears it when that frame is destroyed first. About 20 lines. `reasoned`
- A result value: not from a detached task. A frame that frees itself cannot hold a value for a handle that reads it later. Keeping the frame until the handle reads it is the `std::future` model, and it holds the frame for as long as the handle lives.
  A task that produces a value writes it to an `lv::state`, which is how the rest of the library passes values. `co_await task<T>` for a child that returns a value exists today.

**Recommendation:** `running()`, `if (job)`, `cancel()`, `detach()` first. `co_await job.done()` in a second step, when a use appears. No result value.

### 5. Pause and resume

LVGL has `lv_timer_pause` and `lv_timer_resume`, but a task also waits for events and states, which have no pause.
`lv_timer_resume` does not change the timer's last run: the time spent paused still counts, and the timer fires at once if its period passed. `source` (`lv_timer.c`)
`lv_anim_pause` and `lv_anim_resume` exist and freeze an animation's clock, but they take the animation's pointer. `source` (`lv_anim.h`)
The owner chose to pause the tasks of a tree while its display is hidden, and not on the handle. See "A tree's tasks and the visibility of its display".

### 6. How it composes

- **Owner and handle together.** `lv::start(work, owner)` ties the task to the widget as `lv::spawn(work, owner)` does. The job reports `running() == false` after the owner was deleted. Both ends win.
- **`lv::first`.** A job is awaited through `done()`, as one item of `lv::first(a.next().clicked, job.done())`.
- **A task that spawns tasks.** Today the child is independent: the end of the parent leaves it running. Option: a parent's end cancels its children, as Trio and Kotlin do.
  It needs a list of children in every promise (8 to 16 bytes) and a parent pointer. With the owning job the same result costs nothing: the parent keeps its children's jobs in locals, and its destruction ends them.

```cpp
lv::task<> page(lv::view<"panel"> panel) {
  auto blink = lv::start(blink_lamp());     // ends when `page` ends, however it ends
  co_await panel.next().gone;
}
```

**Recommendation:** no automatic parent and child. Structure comes from where the job lives.

### 7. Where a handle may live

| Place | Works | Note |
|---|---|---|
| A local of a coroutine | Yes | Structured: the task ends with the frame. The best place |
| A member of a class | Yes, if the class moves or never moves | A moved job re-links the task |
| A `constinit` global | Yes | The constructor is `constexpr`. At exit, `lv::application` has ended the tasks before the global's destructor runs; the back-link was cleared, so the destructor does nothing. `reasoned` |
| The storage of a node of a mounted tree | Yes | See below |

**A node that runs a task while its tree is mounted.**
A node kind may declare a `storage` that `lv::mounted` holds.
It is built before the widgets and destroyed while they all exist. `source` (`include/lv/tree.hpp`)
A `run` node would keep an owning `lv::job` in that storage.

```cpp
inline constexpr auto ui = lv::column({...})[
    lv::label({.text = "lvgl26"}),
    lv::run<demo::tick>,                       // starts on mount, ends on unmount
    lv::run<follow_runner, "runner">];         // a function of the widget with this id
```

- The function is a template argument, since a tree is constant: a function pointer, as for handlers. `source` (`CLAUDE.md`: a handler in a constexpr tree is a function pointer)
- `lv::run<fn>` is for a task without parameters.
- `lv::run<fn, "id">` is for one that takes the view of a widget of the tree. Its owner is that widget.
- The storage destructor cancels the job. It deletes no widget, as the rule for storage says.
- `unmount()` and a second `mount(parent)` start the task again, with its state in `lv::state`.
- The example's `lv::spawn(demo::tick())` then becomes one line in `demo::ui`, and `app::running` does not need to run it.

**Recommendation:** build the handle first, then the node. The node is the declarative answer, and it needs the handle. `reasoned`

## A tree's tasks and the visibility of its display

This section is for the second pull request.
The owner decided its three open points on 2026-11-19 (decisions 10 to 12 below).
The parts that stay open for the build are in "Open for the build".

The owner's decision 3 below says that a task declared with `lv::run` pauses while the display of its tree is hidden.
It also says that a tree can declare a task that keeps running whether the display is shown or not.
An example is a task that watches for a message and shows the window.

What it rests on, and what is assumed:

- `lv::display` has no notion of being hidden today. `source` (`include/lv/display.hpp`)
- The branch `feat/pointer` of pull request #132 holds the SDL adapter and `lv::pointer`. It has no commit with `hide()` or `show()` yet. `source` (read 2026-11-19)
- Assumed: the window adapter gets `window.hide()`, `window.show()` and a close request.
  The program passes the window to `lv::display` as its panel, as it does now.
  The names below are to be aligned with what #132 builds. See "Open for the build".

### How the display tells that it is hidden

Decided: `lv::display` gets a visible state and the two requests, forwarded to the platform window that is its panel.
The smallest notion is one observable flag on the display, written by the platform.

```cpp
class display {
 public:
  // True while the screen can be seen. Starts true. The platform writes it.
  bool visible() const;
  void set_visible(bool visible);   // from the platform: the window was hidden or shown
  // A request from the program: forwarded to the panel if it can do it. False if it cannot.
  bool show();
  bool hide();
  // ...
};
```

- The flag is read like a state: it has a `detail::watch_list`, so a tree follows it with a `detail::watch_node` in its storage and allocates nothing. `source` (`include/lv/detail/watch.hpp`, `CLAUDE.md`: follow a state through a node in its list)
- `set_visible(false)` also turns off LVGL's invalidation of that display (`lv_display_enable_invalidation(display, false)`), and `set_visible(true)` turns it on and invalidates the screen.
  A hidden display then draws nothing. The function exists in LVGL v9.6 and in the v10 pin. `source`
- The platform window calls `set_visible` when the window really changes: its own `hide()` and `show()`, a minimize, a restore.
  On a device the same pair can mean the backlight or the display sleep. The display does not know which.
- A tree finds its display from its root widget: `lv_obj_get_display` and then the display's driver data, where `lv::display` already stores its own address. `source` (`display.hpp`, line 169)

### How a task asks for the window

Decided: through the display, with `lv::display_of(view).show()` and `.hide()`.
An always-running task has no SDL and must not name it. The request goes through the display.

- `display.show()` and `display.hide()` call the panel's `show()` and `hide()` when the panel has them, found by a `requires` expression like `frame_panel`. Otherwise they return false.
- The window adapter of #132 would give its `show()` and `hide()`. It calls `set_visible` itself once the window is shown, so the flag follows what happened, not what was asked.
- A task reaches its display with `lv::display_of(view)`.

```cpp
lv::task<> watch_messages(lv::view<"panel"> panel) {
  for (;;) {
    const auto message = co_await lv::changed(inbox);
    if (message.urgent) lv::display_of(panel).show();
  }
}
```

A close request of the window stays the program's. The loop of #132 returns false on it today. A program that keeps a tray-like window calls `hide()` instead of ending, and the always-running task shows it again. `reasoned`

### What "paused" means

A paused task is not resumed. Its code does not run, and it holds its place at the `co_await` where it is.
The owner's words for it: "It wakes once, at once".
There is one gate, in `resume_task`: every wake-up of a task goes through it, from an event, a timer, a state, an animation or `lv::first`. `source` (`task.hpp`, the two `done` functions and the two `spawn`s)

| What happens while paused | Decided | Why |
|---|---|---|
| A task started while the display is hidden | The body does not run until the display is shown | Nothing is shown to start for. `measured` |
| `lv::sleep` | The time keeps counting. The first wake-up is held, and the task goes on at once when shown | The gate is one place and needs no memory per source. A 5 second timeout that ran out while hidden has run out |
| A loop that sleeps and counts | One step after the display is shown, not one for each period that passed | The source is disarmed after its first wake-up, so nothing piles up. `measured` |
| An event that arrives | The first one is held and delivered on resume. Events after it are not heard, as the wait is over | A hidden window gets no pointer input from the platform, so this is a program's own event |
| A state the task waits for | The wake-up is held, and the task reads the state's value when it resumes: the latest, not the first | `await_resume` of `lv::changed` reads the state when it runs. `measured`: three changes while paused gave the value of the third |
| An animation the task started | It runs to its end inside LVGL, and the task goes on at once when shown | With invalidation off nothing is drawn, so it costs property writes only |
| `lv::first(a, b)` | The first of them to be done is held | `lv::first` has already cancelled the others |
| The task is cancelled while paused (its owner or tree is deleted) | It ends as usual, its destructors run, and the held wake-up is dropped | Ending is not running its code. The gate must be cleared by the promise's destructor |
| A job kept by the program (`lv::start`) | Not paused. Only the tasks a tree declares are | The owner did not ask to pause them |
| A `lv::start` task that waits on the tree's widgets | Not paused | The same |

Two alternatives were weighed.

- **The remaining time of a sleep freezes.** The task would see time pass only while shown. It needs the start tick in each sleep (4 bytes).
  It stops and restarts the LVGL timer on each pause, and needs one such hook for every kind of wait.
  Cost: about 40 lines and 4 bytes a sleep. Recommendation: not now. Add it for sleeps alone if a timeout that spans a hidden period turns out to matter.
- **Events are not delivered while paused.** The task would disarm every wait on pause and arm it again on resume. It needs a pause and a resume hook on every source and loses the first change of a state.
  Recommendation: no. The gate gives the same result for states with less code.

A pause is a property of the task, not of the handle: the gate pauses the task that the tree runs, and `job` has no `pause()`.

### How a tree declares a task that runs hidden

The common case stays the shortest, and the other is readable where it is used.

**Option A: a tag in the template arguments.**

```cpp
lv::run<demo::tick>                         // pauses while the display is hidden
lv::run<watch_messages, lv::always>         // runs whether the display is shown or not
lv::run<follow_runner, "runner">            // a task of one widget, paused
lv::run<follow_runner, "runner", lv::always>
```

**Option B: a second name.**

```cpp
lv::run<demo::tick>
lv::run_always<watch_messages>
```

**Option C: a property in the node, as the other nodes take them.**

```cpp
lv::run<watch_messages>({.hidden = "run"})  // "pause" is the default
```

| | A: `lv::always` | B: `lv::run_always` | C: a property |
|---|---|---|---|
| Common case | `lv::run<fn>` | `lv::run<fn>` | `lv::run<fn>` |
| The other case reads | "run it always" | "run always" | "when hidden, run" |
| New names | `lv::always` | `lv::run_always` | none, a property of the node |
| Where it is checked | A concept on the pack of template arguments | Two nodes to keep alike | The existing property checks |
| Fits `lv::look`, which reads names | Yes | Yes | Yes, the enumerator in quotes follows `CLAUDE.md` |

**Decided: A.** One node name, and the tag is visible beside the function it changes.
A tag can later be joined by others without a new node name.
A concept on the pack names a wrong argument in a compile error.
Option C reads as well but puts a run-time looking property on a node whose function is a template argument.
Cost of A: a node with a pack of template arguments, one concept, and a compile-fail test for each wrong argument.

### What it costs

The gate was probed in the same scratch header. The compile cost of the gate was not measured separately.

| What | Result |
|---|---|
| A task of a tree: pointer to its gate | 8 bytes in the frame, on top of the 8 of the job's back-link |
| The gate: `paused` and the held handle | 16 bytes in the node's storage |
| The watcher of the display's flag | A `detail::watch_node`, 32 bytes on a 64-bit target, in the node's storage |
| Allocations | None |

What the probe showed with a sleeping loop of 10 ms, a state and a task:

- Running 35 ms gave 3 steps. Paused for 100 ms it gave no step and held one wake-up. Resumed it gave one step, not ten.
- A state set to 2, 3 and 4 while paused: the task saw 4 when resumed, and 5 for the next change.
- A task cancelled while paused ended, ran its destructor, and was not resumed.
- A task started paused did not run its body until the gate was opened.
- The probe also showed a defect to avoid: the first version left the held handle in the gate after a cancel, so a later resume would have touched a destroyed frame.
  The promise's destructor must clear the gate's held handle, and the job must be destroyed before the gate in the storage.

### Errors

| Mistake | Becomes |
|---|---|
| `lv::run<fn, lv::always, lv::always>` or an argument that is neither an id nor `lv::always` | A compile error by the concept, with the list of what is valid |
| `lv::run<fn, "id">` where the tree has no widget with that id | A compile error that lists the ids, as `get<"id">` does |
| `lv::run<fn>` of a function that is not `lv::task<>()` or `lv::task<>(view)` | A compile error by a concept on the function |
| `display.show()` on a display whose panel cannot show | Returns false. It is a run-time fact of the platform, not a bug |
| A tree mounted on a display that is hidden | Not an error: its tasks start paused |

### Open for the build

- **A required test.** The probe's first version left the held handle in the gate when a task was cancelled while paused. A later resume would have touched a destroyed frame.
  The promise's destructor clears the gate's held handle, and the build needs a test for it: cancel a paused task, then open the gate.
- **The names.** `visible`, `set_visible`, `show` and `hide` of the display are to be aligned with what pull request #132 builds for its window. The two builds meet there.
- **The cost.** The compile cost of the gate and of the `run` node, and the size of the gate on a 32-bit target, are measured in the build.
- **Devices.** How a device maps hide and show (a backlight, a sleep mode) is not studied.

## The order of the work

The owner decided two pull requests, after the open work lands.

1. `lv::start` and `lv::job`: the back-link, the owning handle, `[[nodiscard]]`, `detach()`, the tests, the stubs and `api_uses.cpp`, the docs.
2. `lv::run` with the pause: the display's visible flag and `show()` and `hide()`, the gate, the node with its storage, the tag `lv::always`, and the example moved to `lv::run<demo::tick>`.

The second pull request reads the commits of #132 first.
The pause and always-running part goes back to the owner before it is built.

## What was probed

A scratch copy of `task.hpp` outside the tree got a back-link and a class `lv::job`: `start`, `running`, `cancel`, `detach`-less, move and a destructor that only unlinks (the non-owning kind).
It was compiled with GCC 16.2 for the host against LVGL v9.6.0 and run. The probe was thrown away. `measured`

| What | Result |
|---|---|
| `sizeof(lv::job)` | 8 bytes |
| LVGL heap for a task that sleeps, `lv::spawn` today | 216 bytes |
| The same through `lv::start` | 224 bytes: 8 more, the pointer in the promise. No second allocation |
| `job.cancel()` | The frame was destroyed and its local's destructor ran; `running()` was false; the heap returned to its value before |
| A task that ends by itself | `running()` turned false |
| A moved job | The old job was empty, the new one cancelled the task |
| A dropped (non-owning) job | The task went on ticking |
| Compile cost, a file with `<lv/lv.hpp>` and one `spawn` | +0.08 to +0.10 G instructions on 37.9 G, near the noise of 0.1 G |

An owning job differs by one line in the destructor and has the same bytes. It was not built.
The back-link on the target is the same 8 bytes on a 32-bit pointer: 4 in the promise and 4 in the handle. `reasoned`

## Errors

| Mistake | Becomes | Message, in short |
|---|---|---|
| `lv::start(...)` as a statement, dropping the job | A warning, an error under `-Werror`, by `[[nodiscard]]` | "ignoring return value: the job ends the task when it is dropped" |
| Copying a job | A compile error: `job(const job&) = delete("a job is the only handle of its task: move it, or call detach()")` | |
| `lv::start` of a `task<int>` | A compile error with a deleted overload | "a started task has nowhere to put a result: write it to an lv::state, or co_await it from another task" |
| `co_await job.done()` from a coroutine that is not an lvgl26 task | A compile error by the concept `task_promise_type`, as for every wait today | |
| `job.cancel()` or `job.running()` on an empty or ended job | Neither: a no-op and false. Asking is allowed | |
| `lv::start` when the frame cannot be allocated | A run-time condition: an empty job, `running()` false, as `lv::spawn` returns false | |
| `lv::run<fn>` of a function that is not `lv::task<>()` | A compile error by a concept on the template parameter | |

Every new message gets a test in `tests/fail/`, as `CLAUDE.md` asks.

## Recommendation

1. Add `lv::job` and `lv::start(task<void>)` and `lv::start(task<void>, obj owner)`. The job is move-only, owning, `[[nodiscard]]`, with `running()`, `explicit operator bool`, `cancel()` and `detach()`.
2. Keep `lv::spawn` and its tests unchanged.
3. Use a back-link of 8 bytes in the promise. No allocation.
4. Cancel as today: destructors only.
5. Add `co_await job.done()` when a use appears. Add no result value, no `job.pause()` and no automatic children.
6. After that, add the node `lv::run<fn>` for a task that lives with a tree, with the pause of a later section.
   Change the example to it.

## Not confirmed

- Whether P2300, P3149R11 and P3552R3 were adopted into C++26, and at which meeting. The pages read call P3149R11 "approved for C++26 by WG21", and the draft pages are a draft of the standard, not a published one. `summary`
- What `~stop_callback` does while a callback runs on another thread.
- What dropping a Kotlin `Job` does: no page says it, and the table gives the inference.
- Cooperative cancellation in Kotlin and Swift: the pages that state it were not fetched in full, and the quotes come from other pages of the same documentation.
- Embassy: the effect of dropping a `SpawnToken`, outside cancellation, and a join. No page read says it.
- Whether QCoro's `Task` starts eagerly, what dropping it does, and what happens to a suspended coroutine when its `QObject` dies. The page read says none of it.
- SwiftUI's `.task` and Android's `lifecycleScope`: not read.
- tokio: that an aborted task gets no cleanup beyond the destructors of its state. `summary`
- LVGL: `LV_EVENT_DELETE` order against other handlers, and deleting a timer inside its own callback; `task.hpp` relies on `lv_timer_set_repeat_count(timer, 1)` for that.
- The probe ran on the host only. The 32-bit size is reasoned. The owning job, `done()`, the `run` node and the tests were not built.
- Nothing here was checked against LVGL v10 beyond the names of the timer, animation and display functions.
- The pause: what #132 builds for `hide()` and `show()`, which was not pushed when this was written. The names `visible`, `set_visible`, `show` and `hide` of the display are to be aligned with it.
- The compile cost of the gate and of the `run` node, and the size of the gate on a 32-bit target. Only the host probe ran.
- Whether a platform window reports hide, minimize and restore to the adapter in the way `set_visible` needs. SDL's events were not read.
- How a device maps `hide()` and `show()` (a backlight, a sleep mode): not studied.

## Decisions for the owner

1. **The shape of the handle.**
   Options: non-owning; owning; both under two names.
   Recommendation: both, with `lv::spawn` detached as today and `lv::start` owning.
   Cost: a second name, and `[[nodiscard]]` on the owning one.
2. **Validity of the handle.**
   Options: a back-link; slot and generation; a control block.
   Recommendation: the back-link. It allocates nothing and works with the task region.
   Cost: 8 bytes in each frame and 8 in the handle; a move must re-link.
3. **What a cancelled task can do.**
   Options: destructors only; a status from each `co_await`; a cleanup task.
   Recommendation: destructors only.
   Cost: no await inside cleanup.
4. **A result and waiting.**
   Options: none; `done()` only; a result for `task<T>`.
   Recommendation: `done()` when a use appears; no result.
   Cost: values travel through `lv::state`.
5. **Pause.** Superseded by the owner's decision 3 below: the tasks of a tree pause while its display is hidden.
   Options were: none; `pause()` for sleeps.
6. **Parent and child.**
   Options: independent; a parent cancels its children.
   Recommendation: independent; a job in a local gives the structure.
   Cost: a pointer chain per promise if chosen the other way.
7. **The node `lv::run<fn>`.**
   Options: build it after the handle; never.
   Recommendation: build it after the handle, and move the example to it.
   Cost: a node kind with storage, a concept on the function and two new messages.
8. **Whether the names are `lv::job` and `lv::start`.**
   Options: `lv::job` and `lv::start`; `lv::running` and `lv::run`; `lv::handle` and `lv::launch`.
   Recommendation: `lv::job` and `lv::start`, with `lv::run<fn>` for the node.
   Cost: `run` and `start` are near each other in meaning, and the docs must say which is which.

## What the owner decided

Decided 2026-11-19, on this note.
Where the owner did not decide, the recommendation of this note stands, and he was told so.

| # | Decision |
|---|---|
| 1 | The handle: `lv::spawn` stays as it is. `lv::start(task)`, and with an owner, returns a move-only, owning `lv::job`. Destroying it ends the task. `[[nodiscard]]` catches an accidental drop, and `job.detach()` lets go on purpose. It has `if (job)`, `job.running()` and an idempotent `job.cancel()`. A job kept as a local of a task ends with that task. |
| 2 | A tree declares its tasks: `lv::run<task>` is a tree node. It starts the task on mount and ends it on unmount, with the job in the tree's storage, so `main` needs no `lv::spawn`. It is built after the handle. |
| 3 | Pause: a task declared with `lv::run` pauses while the display of its tree is hidden, and goes on when it is shown. A tree can declare a task that keeps running whatever the display shows, for example one that watches for a message that shows the window. There is no `job.pause()` for now. The spelling, the meaning of "paused", how the tree learns that its display is hidden and how a task asks for the window are decided in 10 to 12. |
| 4 | The order: two pull requests, after the open work lands. First `lv::start` and `lv::job`, then `lv::run` with the pause. |
| 5 | Not decided, so the recommendation stands: validity of the handle by a back-link from the frame, with no allocation and with the later task region. |
| 6 | Not decided, so the recommendation stands: cancellation as today, destructors only. |
| 7 | Not decided, so the recommendation stands: no result value from a detached task and no `co_await job.done()` yet. |
| 8 | Not decided, so the recommendation stands: no automatic cancelling of children beyond what a job held in a parent's local gives. |
| 9 | Not decided: the names `lv::job` and `lv::start`, and `lv::run`. They stand unless he says otherwise. |
| 10 | A task that keeps running whatever the display shows: `lv::run<fn, lv::always>`. `lv::run<fn>` pauses while its display is hidden. |
| 11 | Paused time: "It wakes once, at once". Time keeps counting while paused. A sleep that ended while the display was hidden completes when it is shown, and the loop runs once, not once for each missed period. A state that changed several times is seen at its latest value. There is no frozen remaining time. |
| 12 | Asking for the window: through the display, `lv::display_of(view).show()` and `.hide()`. `lv::display` gets a visible state and the two requests, forwarded to the platform window that is its panel. |
| 13 | Kept open for the build: the cancelled-while-paused defect of the probe as a required test; the names `visible`, `set_visible`, `show` and `hide`, to be aligned with #132; the compile cost of the gate and its size on a 32-bit target; how a device maps hide and show. |
