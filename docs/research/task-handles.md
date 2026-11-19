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

LVGL has `lv_timer_pause` and `lv_timer_resume`, but a task also waits for events and states, which have no pause. Pausing a sleep alone would be a wrong promise.
For a hidden window the better answer is the owner's: a task tied to a tree ends when the tree is unmounted, and the window builds again when shown, which `unmount()` and `mount(parent)` already allow. State lives in `lv::state`, not in the task.

**Recommendation: no pause.** If a use appears, it is `job.pause()` for sleeps only, named so.

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
5. Add `co_await job.done()` when a use appears. Add no result value, no pause and no automatic children.
6. After that, add the node `lv::run<fn>` for a task that lives with a tree, and change the example to it.

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
- Nothing here was checked against LVGL v10 beyond the names of the timer and animation functions.

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
5. **Pause.**
   Options: none; `pause()` for sleeps.
   Recommendation: none; a tree's task ends with the tree.
   Cost: a task that must keep its place keeps it in a state.
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
