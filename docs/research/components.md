# Components: authoring model, parameters, slots, behaviour

Research for the component design of lvgl26, written 2026-11-16.

Marks follow `docs/research/README.md`. Every mark is dated 2026-11-16 unless its row says otherwise.

- `source`: the linked page or the code was read in this session. A fetch tool extracted the text, so a quote may differ in a word.
- `measured`: compiled or measured in this session. The method is under "Measured here".
- `summary`: a search engine's summary of the page. The page was not read.
- `recalled`: from memory. Verify before relying on it.

LVGL rows were read in the local checkout of v9.6.0. Their links are built from the tag and the path.

## Short answers

| Question | Answer | Main evidence |
|---|---|---|
| A. Authoring model | The static template with structural nodes (`when`, `each`). A re-run builder needs a retained tree, a diff, keys and call-order rules. | Sections 1 and 4 |
| B. Parameters | Types first: group dependent members into one checked value. Then one compile-time check over the whole set. Contracts only where a value arrives at run time. | Section 6, "Measured here" |
| C. Slots | Named slots that are typed members of the component. The type lists the allowed kinds, the count and the data handed to the content. | Section 5 |
| D. Behaviour | Behaviours as constants attached to a node, started in that node's lifetime scope, written as plain handlers or `lv::task` coroutines. | Sections 1, 2 and 3 |

## Measured here

All with Homebrew GCC 16.2.0, source given on standard input, no file written to the repository.

| What | Result | How |
|---|---|---|
| Annotated member functions are found by reflection | `[[=behavior]] task blink();` is counted by `annotations_of_with_type`; a negative control fails as it should | `g++-16 -std=c++26 -freflection -fsyntax-only`, `static_assert` |
| Member coroutines are found by return type | `return_type_of(m) == ^^task` finds three of three | same |
| A required parameter | `required<T>` with `required() = delete("reason")` rejects `{.count = 1}`; GCC prints the reason | same |
| A check over a whole parameter set | A wrapper with a `consteval` constructor calls `check()`; `{.min = 0, .max = 10, .value = 50}` is rejected with our text | same; the sketch used `throw`, the library would use `detail::fail` |
| A contract in constant evaluation | `constexpr range(int lo, int hi) pre(lo <= hi)`; `range{10, 0}` gives "contract predicate is false in constant expression" | same, plus `-fcontracts` |
| Behaviours piped onto a node | `btn \| long_press(&stepper::up) \| fade_in_t{200} \| run_member(&stepper::repeat)` is a constant; `mount<const auto&>` sees three behaviours | same |
| A behaviour on the wrong widget kind | `lbl \| long_press(...)` selects an overload `= delete("lv: this behaviour does not fit this widget ...")` | same |
| Typed slots | `fill(&card::actions)[b, l]` and four buttons in a slot of three fail with a `static_assert` text | same |
| `std::inplace_vector` | The header exists in libstdc++ 16. `try_push_back` returns `std::optional<T&>` | same |
| LVGL struct sizes on Cortex-M4 | `lv_obj_t` 56, `lv_obj_spec_attr_t` 56, `lv_button_t` 56, `lv_label_t` 120, `lv_timer_t` 28, `lv_anim_t` 96, `lv_observer_t` 24, `lv_subject_t` 36, `lv_event_dsc_t` 12 bytes | `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -DLV_CONF_SKIP -fsyntax-only`, LVGL 9.6.0 default configuration, `sizeof` forced into a diagnostic |

The struct sizes exclude the allocator's own header, styles and text.

## 1. Behaviour composition models

### React

| Finding | Link | Mark |
|---|---|---|
| A custom Hook is a function named `use...` that calls other Hooks. It shares stateful logic, not state. | [custom Hooks](https://react.dev/learn/reusing-logic-with-custom-hooks) | `source` |
| Each call of a Hook is independent. Hooks compose by passing one Hook's result to the next. | [custom Hooks](https://react.dev/learn/reusing-logic-with-custom-hooks) | `source` |
| An effect's cleanup runs before each re-run, with the old values, and after the component is removed. | [useEffect](https://react.dev/reference/react/useEffect) | `source` |
| Dependencies are a hand-written array, compared with `Object.is`. | [useEffect](https://react.dev/reference/react/useEffect) | `source` |
| All Hooks re-run on every render. | [custom Hooks](https://react.dev/learn/reusing-logic-with-custom-hooks) | `source` |
| Hooks may not be called in loops, conditions, nested functions or after an early return. | [Rules of Hooks](https://react.dev/reference/rules/rules-of-hooks) | `source` |
| Vue's documentation lists the costs: call-order rules, stale closures, manual dependency arrays, `useMemo`, `useCallback`. | [Composition API FAQ](https://vuejs.org/guide/extras/composition-api-faq.html) | `source` |

### Jetpack Compose

| Finding | Link | Mark |
|---|---|---|
| `LaunchedEffect` launches a coroutine when it enters the composition and cancels it when it leaves. | [Side-effects](https://developer.android.com/develop/ui/compose/side-effects) | `source` |
| Changed keys cancel the running coroutine and launch a new one. | [Side-effects](https://developer.android.com/develop/ui/compose/side-effects) | `source` |
| `DisposableEffect` must end with `onDispose`. The IDE shows a build-time error otherwise. | [Side-effects](https://developer.android.com/develop/ui/compose/side-effects) | `source` |
| `rememberCoroutineScope` returns a scope bound to its call site. It is cancelled when the call leaves the composition. | [Side-effects](https://developer.android.com/develop/ui/compose/side-effects) | `source` |
| Rule: a variable used in an effect is a key, or it is wrapped in `rememberUpdatedState`. | [Side-effects](https://developer.android.com/develop/ui/compose/side-effects) | `source` |
| A Modifier is an ordered, immutable list of elements. Each element is one behaviour. | [Modifiers](https://developer.android.com/develop/ui/compose/modifiers) | `source` |
| Order matters: elements added first are applied first. | [Modifiers](https://developer.android.com/develop/ui/compose/modifiers) | `source` |
| A chain is a "data-like object". Stored in a constant and reused, it saves allocations on each recomposition. | [Modifiers](https://developer.android.com/develop/ui/compose/modifiers) | `source` |
| A `Modifier.Node` holds state and survives recompositions. Its `ModifierNodeElement` is stateless and allocated at each recomposition. | [Custom modifiers](https://developer.android.com/develop/ui/compose/custom-modifiers) | `source` |
| A node has `onAttach`, `onDetach` and its own `coroutineScope`. | [Custom modifiers](https://developer.android.com/develop/ui/compose/custom-modifiers) | `source` |
| Node kinds are interfaces: draw, layout, pointer input, semantics, parent data. | [Custom modifiers](https://developer.android.com/develop/ui/compose/custom-modifiers) | `source` |
| A `DelegatingNode` composes several nodes into one and lets them share state. | [Custom modifiers](https://developer.android.com/develop/ui/compose/custom-modifiers) | `source` |
| `composed {}` is no longer recommended, for performance. `Modifier.Node` replaced it. | [Custom modifiers](https://developer.android.com/develop/ui/compose/custom-modifiers) | `source` |
| Some modifiers exist only inside the right parent: `weight` in `RowScope`, `matchParentSize` in `BoxScope`. | [Modifiers](https://developer.android.com/develop/ui/compose/modifiers) | `source` |

### SwiftUI

| Finding | Link | Mark |
|---|---|---|
| `.task(id:)` adds a task that starts before the view appears. | [task(id:)](https://developer.apple.com/documentation/swiftui/view/task(id:name:priority:file:line:_:)) | `source` |
| SwiftUI cancels it after the view disappears. It cancels and restarts it when `id` changes. `id` is `Equatable`. | [task(id:)](https://developer.apple.com/documentation/swiftui/view/task(id:name:priority:file:line:_:)) | `source` |
| The action runs synchronously until its first `await`. | [task(id:)](https://developer.apple.com/documentation/swiftui/view/task(id:name:priority:file:line:_:)) | `source` |
| `onAppear` runs its action before the first rendered frame. | [onAppear](https://developer.apple.com/documentation/swiftui/view/onappear(perform:)) | `source` |
| A `ViewModifier` is a type with `body(content:)`. It is applied with `modifier(_:)`, usually behind an extension on `View`. | [ViewModifier](https://developer.apple.com/documentation/swiftui/viewmodifier) | `source` |

### Flutter

| Finding | Link | Mark |
|---|---|---|
| The State lifecycle is `initState`, `didChangeDependencies`, `build`, `didUpdateWidget`, `deactivate`, `dispose`. | [State](https://api.flutter.dev/flutter/widgets/State-class.html) | `source` |
| A State is kept while its slot holds a widget of the same `runtimeType` and key. | [State](https://api.flutter.dev/flutter/widgets/State-class.html) | `source` |
| flutter_hooks exists because the logic of `initState` and `dispose` is hard to reuse. | [flutter_hooks](https://pub.dev/packages/flutter_hooks) | `source` |
| Its page names the limits of mixins: one use per class, and members with the same name conflict. | [flutter_hooks](https://pub.dev/packages/flutter_hooks) | `source` |
| Hooks live in a list in the Element and are found by call index. The call order must not change. | [flutter_hooks](https://pub.dev/packages/flutter_hooks) | `source` |

### Svelte

| Finding | Link | Mark |
|---|---|---|
| An attachment is a function that runs in an effect when the element is mounted. | [@attach](https://svelte.dev/docs/svelte/@attach) | `source` |
| It may return a cleanup. That runs before a re-run and after the element is removed. | [@attach](https://svelte.dev/docs/svelte/@attach) | `source` |
| It re-runs when state that it read changes. | [@attach](https://svelte.dev/docs/svelte/@attach) | `source` |
| A factory such as `tooltip(content)` returns an attachment. An element can have any number of them. | [@attach](https://svelte.dev/docs/svelte/@attach) | `source` |
| Attachments pass through a component as props with Symbol keys. | [@attach](https://svelte.dev/docs/svelte/@attach) | `source` |
| An action (`use:`) runs once at mount. It does not run again when its argument changes. | [use:](https://svelte.dev/docs/svelte/use) | `source` |
| Svelte advises attachments over actions since 5.29: "more flexible and composable". | [use:](https://svelte.dev/docs/svelte/use) | `source` |
| `$effect` tracks what it reads synchronously. It is destroyed with its parent effect or component. | [$effect](https://svelte.dev/docs/svelte/$effect) | `source` |

### Vue and Solid

| Finding | Link | Mark |
|---|---|---|
| A Vue composable is a function that holds reusable stateful logic. Composables call each other. | [Composables](https://vuejs.org/guide/reusability/composables.html) | `source` |
| Each component instance that calls it gets its own state. | [Composables](https://vuejs.org/guide/reusability/composables.html) | `source` |
| It hooks into the owner's lifecycle (`onUnmounted`). It must be called synchronously in setup. | [Composables](https://vuejs.org/guide/reusability/composables.html) | `source` |
| Vue no longer recommends mixins: unclear source of properties, namespace collisions, implicit cross-mixin communication. | [Composables](https://vuejs.org/guide/reusability/composables.html) | `source` |
| Vue's `setup()` runs once. Calls are not order-sensitive and can be conditional. | [Composition API FAQ](https://vuejs.org/guide/extras/composition-api-faq.html) | `source` |
| A Solid component function runs once. Later changes update only the affected parts. | [Components](https://docs.solidjs.com/concepts/components/basics) | `source` |
| Solid's `onCleanup` registers a cleanup on the current reactive scope. It runs when the scope is disposed or re-run. | [onCleanup](https://docs.solidjs.com/reference/lifecycle/on-cleanup) | `source` |
| `use:name={value}` calls `name(element, accessor)` before the element is connected. | [use:*](https://docs.solidjs.com/reference/jsx-attributes/use) | `source` |
| `use:` works only on native elements. It is not forwarded through components. | [use:*](https://docs.solidjs.com/reference/jsx-attributes/use) | `source` |

### QML and Slint

| Finding | Link | Mark |
|---|---|---|
| QML: `Behavior on width { NumberAnimation {} }` gives a property a default animation for every change. | [Behavior](https://doc.qt.io/qt-6/qml-qtquick-behavior.html) | `source` |
| QML: a property cannot have more than one Behavior. | [Behavior](https://doc.qt.io/qt-6/qml-qtquick-behavior.html) | `source` |
| QML: a matching Transition overrides the Behavior for that state change. | [Behavior](https://doc.qt.io/qt-6/qml-qtquick-behavior.html) | `source` |
| QML: a State is a named set of property changes. `when` binds it to an expression. | [States](https://doc.qt.io/qt-6/qtquick-statesanimations-states.html) | `source` |
| QML: attached properties and handlers annotate an object from outside: `ListView.isCurrentItem`, `Component.onCompleted`. | [Object attributes](https://doc.qt.io/qt-6/qtqml-syntax-objectattributes.html) | `source` |
| QML: `Connections` handles signals of another object and allows several handlers for one signal. | [Connections](https://doc.qt.io/qt-6/qml-qtqml-connections.html) | `source` |
| QML: `Timer` has `interval`, `repeat` and `running`. Its resolution is at best 16 ms. | [Timer](https://doc.qt.io/qt-6/qml-qtqml-timer.html) | `source` |
| QML: these are objects declared inside an item, and they die with it. | none | `recalled` |
| Qt for MCUs: states, the `state` property and transitions exist only on the root item of a QML file. | [API differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-api-differences.html) | `source` |
| Slint: `states [ name when condition : { property: value; } ]`. One block sets properties of several elements. | [States](https://docs.slint.dev/latest/docs/slint/guide/language/coding/states/) | `source` |
| Slint: transitions are `in`, `out` and `in-out` blocks that hold `animate`. | [States](https://docs.slint.dev/latest/docs/slint/guide/language/coding/states/) | `source` |
| Slint: `animate x, y { duration: 100ms; }` plays on every change of the property, whatever caused it. | [Animation](https://docs.slint.dev/latest/docs/slint/guide/language/coding/animation/) | `source` |
| Slint: `Timer` is a non-visual element with `interval`, `running` and `triggered()`. It runs by default. | [Timer](https://docs.slint.dev/latest/docs/slint/reference/timer/) | `source` |
| Slint: a `changed` callback fires only on a real change. A loop is broken after a few iterations. | [Reactivity](https://docs.slint.dev/latest/docs/slint/guide/language/concepts/reactivity/) | `summary` |

### Elm, TouchGFX, LVGL

| Finding | Link | Mark |
|---|---|---|
| Elm: a program returns `Cmd` and `Sub` values. The runtime performs the effects and sends messages back. | [Effects](https://guide.elm-lang.org/effects/) | `source` |
| Elm: `subscriptions : Model -> Sub Msg`. What is subscribed is a function of the model. | [Time](https://guide.elm-lang.org/effects/time.html) | `source` |
| TouchGFX: the presenter fetches from the model and formats for a passive view. | [MVP](https://support.touchgfx.com/docs/development/ui-development/software-architecture/model-view-presenter-design-pattern) | `source` |
| TouchGFX: the framework calls one function every tick, for the backend. | [MVP](https://support.touchgfx.com/docs/development/ui-development/software-architecture/model-view-presenter-design-pattern) | `source` |
| TouchGFX: a View gets ticks. A container must call `registerTimerWidget(this)` and unregister itself. | [forum](https://community.st.com/t5/stm32-mcus-touchgfx-and-gui/how-can-i-use-handletickevent-in-custom-cantainer/m-p/187095) | `summary` |
| TouchGFX allocates nothing at run time. One screen's objects live in a block sized for the largest screen. | [Memory usage](https://support.touchgfx.com/docs/basic-concepts/memory-usage) | `source` |
| LVGL: a timer is a record in a list with `user_data`, `repeat_count` and `auto_delete`. It names no widget. | [lv_timer_private.h](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_timer_private.h) | `source` |
| LVGL: `lv_anim_start` copies the descriptor into a list. | [lv_anim.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_anim.c) | `source` |
| LVGL: deleting a widget deletes its animations (`lv_anim_delete(obj, NULL)` in the destructor). | [lv_obj.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj.c) | `source` |
| LVGL: an observer bound to a widget is removed on that widget's `LV_EVENT_DELETE`. | [lv_observer.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_observer.c) | `source` |
| LVGL: an event goes to the parent if the widget has the bubble flag. Trickle sends it to the children. | [lv_obj_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_event.c) | `source` |
| LVGL: event handlers are kept in an array, appended in the order they are added. | [lv_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_event.c) | `source` |
| LVGL: handlers run in that order, the preprocess ones first. | none | `recalled` |
| LVGL on Cortex-M4: a timer is 28 bytes, an animation 96, an observer 24, an event handler 12. | "Measured here" | `measured` |

### The models side by side

| Model | Unit of reuse | Attached by | Ends when | Several on one element | Cost at run time |
|---|---|---|---|---|---|
| React Hooks | Function `useX` | A call in the component body | Unmount, or a dependency changes | By call order; nest by calling | Re-run on every render |
| Compose effects | Composable function | A call in the composition | It leaves the composition, or a key changes | By call site | Restart when a key changes |
| Compose modifiers | Element value and stateful node | A chain on the element | The node is detached | Ordered chain; delegation | An element per recomposition unless hoisted |
| SwiftUI | `ViewModifier`, `.task` | A modifier chain | The view disappears, or `id` changes | Chain order | No number found |
| Flutter | State, mixin, hook | The class; `use` in `build` | `dispose` | Hooks by call index | Rebuild |
| Svelte attachments | Function, factory | `{@attach f(x)}` | The element is removed, or state changes | Any number | One effect each |
| Vue composables | Function | A call in setup, once | The owner unmounts | Call several; nest | Subscriptions only |
| Solid | Function, directive | A call, once; `use:` | The owner scope is disposed | Not stated | Subscriptions only |
| QML | Child object | Declared inside the item | With the item | One Behavior per property | One object each |
| Slint | `animate`, `states`, `Timer` | Declared in the element | With the element | Per property | Compiled code |
| Elm | `Sub` value | Returned from `subscriptions` | The model stops returning it | A batch (`recalled`) | The runtime compares subscriptions (`recalled`) |
| TouchGFX | Virtual `handleTickEvent` | Override, then register | Unregistered by hand | One per class | A call per frame |
| LVGL | Timer, animation, observer, handler | A C call with `user_data` | The widget is deleted; a timer never | Array order | 12 to 96 bytes each |

What the table shows.

- Every model ties a behaviour to a view's lifetime and cleans up automatically. Only TouchGFX ticks and LVGL timers do not.
- Two shapes won: a value attached to an element (modifier, attachment), and a function called once in a scope (composable).
- Frameworks that re-run the component need call-order rules and dependency arrays. Frameworks that run it once do not.
- Class-based reuse was abandoned: Vue dropped mixins, flutter_hooks names their limits, Svelte moved from actions to attachments.

## 2. Structured concurrency and lifetimes

| Finding | Link | Mark |
|---|---|---|
| Trio: a child task starts only in a nursery. The nursery block exits when all children have finished. | [Trio core](https://trio.readthedocs.io/en/stable/reference-core.html) | `source` |
| Trio: a failing child cancels the others, and the error is raised again in the parent. | [Trio core](https://trio.readthedocs.io/en/stable/reference-core.html) | `source` |
| Trio: cancel scopes nest. Cancellation arrives at the next checkpoint and keeps being raised. | [Trio core](https://trio.readthedocs.io/en/stable/reference-core.html) | `source` |
| "Tasks form a tree." A function whose task must outlive it takes a nursery as an argument. | [Smith, 2018](https://vorpus.org/blog/notes-on-structured-concurrency-or-go-statement-considered-harmful/) | `source` |
| Kotlin: Jobs form a parent-child tree. Cancelling a parent cancels all children recursively. | [Job](https://kotlinlang.org/api/kotlinx.coroutines/kotlinx-coroutines-core/kotlinx.coroutines/-job/) | `source` |
| Kotlin: a failing child cancels its parent, unless a `SupervisorJob` is used. A parent waits for its children. | [Job](https://kotlinlang.org/api/kotlinx.coroutines/kotlinx-coroutines-core/kotlinx.coroutines/-job/) | `source` |
| Kotlin's guide puts 50,000 waiting coroutines at roughly 500 MB. That is about 10 KB each. | [Coroutines basics](https://kotlinlang.org/docs/coroutines-basics.html) | `source` |
| Swift: a child task does not outlive the scope that created it. A task group awaits all its tasks. | [SE-0304](https://github.com/swiftlang/swift-evolution/blob/main/proposals/0304-structured-concurrency.md) | `source` |
| Swift: cancellation is cooperative; code checks `Task.isCancelled`. `Task {}` is unstructured. | [SE-0304](https://github.com/swiftlang/swift-evolution/blob/main/proposals/0304-structured-concurrency.md) | `source` |
| C++26: `counting_scope` counts spawned operations. `join()` completes when the count reaches zero. | [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) | `source` |
| C++26: `counting_scope` adds a stop source. `simple_counting_scope` only counts. | [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) | `source` |
| C++26: `spawn` allocates the operation state dynamically, with an allocator from the environment. | [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) | `source` |
| C++26: a scope destroyed in the wrong state calls `std::terminate`. | [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) | `source` |
| cppreference lists `when_all`, `counting_scope`, `spawn` and `task`. It lists no `when_any`. | [std::execution](https://en.cppreference.com/w/cpp/execution) | `source` |
| `inplace_stop_source` owns its stop state alone. It avoids the heap control block of `std::stop_token`. | [inplace_stop_token](https://en.cppreference.com/w/cpp/thread/inplace_stop_token) | `source` |
| `std::execution::task` expects at least one allocation, for the frame. An allocator comes through `allocator_arg`. | [P3552R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3552r3.html) | `source` |
| A coroutine frame is allocated with `operator new`. A promise type can replace it. | [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) | `source` |
| The frame holds the promise, copies of the parameters, the suspension point and locals that live across one. | [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) | `source` |
| The allocation may be removed when the frame's lifetime is nested in the caller and its size is known there. | [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) | `source` |
| With `get_return_object_on_allocation_failure`, a failed allocation returns an object and throws nothing. | [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) | `source` |
| That elision (HALO) needs the ramp function and small wrappers inlined. The paper names only Clang. | [P0981R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0981r0.html) | `source` |
| GCC does not elide frames: PR 100579 (2021) and PR 115309 (2024). | [PR 100579](https://inbox.sourceware.org/gcc-bugs/bug-100579-4@http.gcc.gnu.org%2Fbugzilla%2F/T), [PR 115309](https://gcc.gnu.org/pipermail/gcc-bugs/2024-May/867270.html) | `summary` |
| The size of a frame is not available as a constant in the program. | none | `recalled` |
| lvgl26: frames come from `lv_malloc` through the promise. A failed allocation gives an empty task. | `include/lv/task.hpp` | `source` |
| lvgl26: a task is tied to a widget only after it waited for an event or an animation of it. | `include/lv/task.hpp`, `include/lv/animate.hpp` | `source` |
| lvgl26: a task that only sleeps, or only waits for a state, has no owner. Nothing cancels it. | `include/lv/task.hpp` | `source` |
| lvgl26: cancelling destroys the suspended frame at once. A running task is destroyed at its next suspension. | `include/lv/task.hpp` | `source` |
| lvgl26: each widget a task depends on costs one more small allocation (`widget_watch`). | `include/lv/task.hpp` | `source` |
| lvgl26 on Cortex-M4: a minimal task is 128 bytes, an animation task 1,104 bytes. | `docs/research/cpp26.md` | `measured` 2026-11-15, not repeated |

What fits one UI thread on a microcontroller.

- LVGL's widget tree already is a scope tree: a parent owns its children, and `LV_EVENT_DELETE` is the scope's end.
- On one thread, cancellation can be the destruction of a suspended frame. No stop token, atomics or checkpoints are needed.
- lvgl26 has this, but only after the first wait on a widget. A component needs ownership from the start.
- Every task is one allocation, because GCC does not elide frames. A plain event handler costs 12 bytes instead.
- A fixed pool is possible through the promise's `operator new`. Its block sizes come from measurement, not from the compiler.
- `counting_scope` solves a multi-threaded problem. Its `spawn` allocates too. It was not measured here.

## 3. Behaviours as compile-time values in C++

| Finding | Link | Mark |
|---|---|---|
| A range adaptor closure composes with a pipe: `C \| D` is again a closure, and `R \| C` equals `C(R)`. | [RangeAdaptorClosureObject](https://en.cppreference.com/w/cpp/named_req/RangeAdaptorClosureObject) | `source` |
| GCC 16.2: behaviours piped onto a node form one constant that a `const auto&` template parameter can read. | "Measured here" | `measured` |
| GCC 16.2: a behaviour on a widget of the wrong kind is a compile error with our text. | "Measured here" | `measured` |
| Compose reaches the same check with receiver scopes: a scoped modifier does not exist outside its parent. | [Modifiers](https://developer.android.com/develop/ui/compose/modifiers) | `source` |
| Deducing `this` removes the template from CRTP: a mixin base "doesn't even need to be a template". | [P0847R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) | `source` |
| A mixin is still a base class: one use per class, shared names. Vue and Flutter left that model. | [Composables](https://vuejs.org/guide/reusability/composables.html), [flutter_hooks](https://pub.dev/packages/flutter_hooks) | `source` |
| A designated initializer cannot name a member of a base class. | none | `recalled` |
| Annotations (`[[=expr]]`) attach a structural constant to a member function. `annotations_of_with_type` reads it. | [P3394R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html) | `source` |
| GCC 16.2 finds annotated member coroutines of a struct by reflection. | "Measured here" | `measured` |
| Boost.SML: a state machine is a transition table, `state + event [guard] / action = state`, checked at compile time. | [SML overview](https://boost-ext.github.io/sml/overview.html) | `source` |
| Boost.SML's own benchmark: 1 byte for a simple machine, 2 for a composite one (Clang 3.7). | [SML benchmarks](https://boost-ext.github.io/sml/benchmarks.html) | `source` |
| SCXML is the W3C form of Harel statecharts: compound and parallel states, history, entry and exit actions. | [SCXML](https://www.w3.org/TR/scxml/) | `source` |
| SCXML: when several transitions match, the first in document order is taken. | [SCXML](https://www.w3.org/TR/scxml/) | `source` |
| Rx debounce: emit an item only if a time span passed without another one. | [Debounce](https://reactivex.io/documentation/operators/debounce.html) | `source` |
| An Rx chain keeps one subscription object for each operator. | none | `recalled` |
| C++26 senders have `when_all`. A `when_any` is not listed. | [std::execution](https://en.cppreference.com/w/cpp/execution) | `source` |
| lvgl26 already has the missing one: `lv::first` returns the winner and cancels the rest. | `include/lv/task.hpp` | `source` |

Which compose best and stay checkable.

| Form | Several on one node | Reuse across components | Checked at compile time | Memory |
|---|---|---|---|---|
| Constants piped onto a node | Yes, in order | Yes: a behaviour is a named constant | Yes: a concept per widget kind | Constants; a handler or a frame per instance |
| `.behaviors = {...}` member | Yes | Yes | Weaker: one member type means type erasure | The same, plus a table |
| Mixin or CRTP base | No: one per class | Poor: names collide | Yes | State in the object |
| Member coroutines found by reflection | Per component, not per node | No: bound to one struct | Yes: return type and annotation | A frame each |
| State machine as a table | One per property group | Yes, as a value | Yes: unknown states and events | Bytes |
| Event-stream operators | Yes | Yes | Types only | An object per operator, or one frame as a coroutine |

- A debounce written as a coroutine with `lv::first` needs one frame and no operator objects.
- A state machine fits modes of a component. It does not fit one-off sequences, where a coroutine is shorter.

## 4. Dynamic content on constrained devices

| Finding | Link | Mark |
|---|---|---|
| Slint: `for x[i] in model : Element {}`. A model is an integer, an array or a model from the business logic. | [Repetition](https://docs.slint.dev/latest/docs/slint/guide/language/coding/repetition-and-data-models/) | `source` |
| Slint: `if condition : Element {}` instantiates the element only while the condition holds. | [Repetition](https://docs.slint.dev/latest/docs/slint/guide/language/coding/repetition-and-data-models/) | `source` |
| Slint's repeater keeps a vector of `(state, optional instance)` and creates an instance on first need. | [repeater.rs](https://github.com/slint-ui/slint/blob/master/internal/core/model/repeater.rs) | `source` |
| Slint: the model reports `row_added`, `row_removed`, `row_changed`. A changed row updates only its own instance. | [repeater.rs](https://github.com/slint-ui/slint/blob/master/internal/core/model/repeater.rs) | `source` |
| Slint `ListView`: elements are only instantiated if they are visible. | [ListView](https://docs.slint.dev/latest/docs/slint/reference/std-widgets/views/listview/) | `source` |
| Slint on a 264 KB board: it still uses the `alloc` crate. Images and glyphs are pre-rendered at compile time. | [Slint blog, 2022](https://slint.dev/blog/porting-slint-to-microcontrollers) | `source` |
| Qt Quick: `ListView` delegates are created as needed and may be destroyed at any time. Keep no state in them. | [ListView](https://doc.qt.io/qt-6/qml-qtquick-listview.html) | `source` |
| Qt Quick: `reuseItems` pools delegates. Properties not given by the model keep their old values on reuse. | [ListView](https://doc.qt.io/qt-6/qml-qtquick-listview.html) | `source` |
| Qt Quick: `Loader` creates a component on demand and destroys the previous item when the source changes. | [Loader](https://doc.qt.io/qt-6/qml-qtquick-loader.html) | `source` |
| Qt for MCUs: `Repeater` creates all its delegates when it is created. | [Repeater (MCUs)](https://doc.qt.io/QtForMCUs/qml-qtquick-repeater.html) | `source` |
| Qt for MCUs: "ListModels are read-only." | [Non-API differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html) | `source` |
| Qt for MCUs: only a certain number of delegates is created, estimated at compile time. An invisible one is reused. | [Non-API differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html) | `source` |
| Qt for MCUs: a delegate is declared in the file that uses it. All delegates of a `ListView` have one size. | [Non-API differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html), [Known issues](https://doc.qt.io/QtForMCUs/qtul-known-issues.html) | `source` |
| Qt for MCUs: `Loader` takes its items from a separate allocator. It cannot be used in a view delegate. | [Loader (MCUs)](https://doc.qt.io/QtForMCUs/qml-qtquick-loader.html) | `source` |
| TouchGFX `ScrollList`: 20 items are shown with `DrawableListItems<CustomContainer, 6>` and an update callback per index. | [Scroll List](https://support.touchgfx.com/docs/development/ui-development/ui-components/containers/scroll-list) | `source` |
| Compose: an instance is identified by its call site, plus execution order inside a loop. | [Lifecycle](https://developer.android.com/develop/ui/compose/lifecycle) | `source` |
| Compose: without `key`, inserting at the top recomposes every item and restarts its effects. | [Lifecycle](https://developer.android.com/develop/ui/compose/lifecycle) | `source` |
| SwiftUI `ForEach` needs `Identifiable` data or an `id`. Lazy containers want a constant number of views per element. | [ForEach](https://developer.apple.com/documentation/swiftui/foreach) | `source` |
| Svelte: an unkeyed `each` adds and removes at the end. A keyed one inserts, moves and deletes. | [{#each}](https://svelte.dev/docs/svelte/each) | `source` |
| Solid `<For>` maps items by identity and moves a node instead of recreating it. `<Index>` keeps positions fixed. | [For](https://docs.solidjs.com/reference/components/for), [List rendering](https://docs.solidjs.com/concepts/control-flow/list-rendering) | `source` |
| React: an exact tree diff is O(n^3). React uses an O(n) heuristic with types and keys. | [Reconciliation](https://legacy.reactjs.org/docs/reconciliation.html) | `source` |
| Flutter has no tree diff. It matches each child list in O(N) by type and key, with a hash table. | [Inside Flutter](https://docs.flutter.dev/resources/inside-flutter) | `source` |
| Flutter keeps a second tree, the element tree, because widgets are immutable and rebuilt. | [Inside Flutter](https://docs.flutter.dev/resources/inside-flutter) | `source` |
| LVGL: the list widget creates one object per row, plus a label. | [lv_list.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/list/lv_list.c) | `source` |
| LVGL removed the list widget on master. | `docs/research/lvgl-upstream.md` | `source` 2026-11-13 |
| LVGL: a table keeps its cells as text records, not as widgets. | [lv_table_private.h](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/table/lv_table_private.h) | `source` |
| LVGL on Cortex-M4: a button that holds a label is at least 232 bytes of structs. | "Measured here" (56 + 56 + 120) | `measured` |
| lvgl26: `detail::build<Tree, Path...>` is a function for one subtree of a constant tree. | `include/lv/tree.hpp` | `source` |
| lvgl26: a mounted tree keeps one pointer per id, in a `std::array` sized at compile time. | `include/lv/tree.hpp` | `source` |

Which authoring model extends best, and why.

- The frameworks built for microcontrollers all use a static template with structural nodes: Slint, Qt for MCUs, TouchGFX.
- A re-run builder needs a retained second tree, identity rules and a matching pass. Compose, Flutter and React document all three.
- A structural node changes only its own place. Slint's row callbacks and Solid's moved nodes show it needs no diff.
- In lvgl26 a subtree already is a function, `build<Tree, Path...>`. A structural node calls it later, or deletes its widget.
- A re-run builder would lose what the constant tree gives: styles in read-only memory, typed ids, no tree at run time.
- Memory follows three strategies: a fixed capacity, a recycled pool of visible rows, or cells that are not widgets.
- 100 rows of a button with a label need over 23 KB of structs. LVGL's default heap is 64 KiB.
- An id inside a repeated subtree cannot be one pointer. It must be reached through its row.

## 5. Slots and content projection

| Finding | Link | Mark |
|---|---|---|
| Web Components: `<slot name="x">` is filled by children with `slot="x"`. Children without it go to the default slot. | [slot element](https://developer.mozilla.org/en-US/docs/Web/HTML/Reference/Elements/slot) | `source` |
| Web Components: a slot can hold fallback content, shown when nothing is assigned. | [slot element](https://developer.mozilla.org/en-US/docs/Web/HTML/Reference/Elements/slot) | `source` |
| Vue: slot content sees the parent's scope, not the child's. | [Slots](https://vuejs.org/guide/components/slots.html) | `source` |
| Vue: named slots (`#header`), fallback content, and `$slots` to test whether a slot was filled. | [Slots](https://vuejs.org/guide/components/slots.html) | `source` |
| Vue: a scoped slot gets props from the child: "a function being passed into the child component". | [Slots](https://vuejs.org/guide/components/slots.html) | `source` |
| Svelte 5: a snippet is a value with parameters, typed `Snippet<[T]>`, passed as a prop. | [Snippets](https://svelte.dev/docs/svelte/snippet) | `source` |
| Svelte 5 deprecated slots: snippets "are more powerful and flexible". | [Snippets](https://svelte.dev/docs/svelte/snippet) | `source` |
| Compose: a slot is a `@Composable` lambda parameter. Several slots are several named parameters. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| Compose: a receiver scope, `content: @Composable RowScope.() -> Unit`, says how the slot lays out its children. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| Compose: avoid a slot with its own DSL, except where children are composed lazily. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| Compose: slot content should not be disposed and composed again when the parent changes structure. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| SwiftUI: `@ViewBuilder` marks a closure parameter that produces child views, with `if` and `else`. | [ViewBuilder](https://developer.apple.com/documentation/swiftui/viewbuilder) | `source` |
| SwiftUI: `TableColumnBuilder` builds only `TableColumnContent`, with generic constraints on the row type. | [TableColumnBuilder](https://developer.apple.com/documentation/swiftui/tablecolumnbuilder) | `source` |
| Slint: `@children` says where a component's children are placed. Without it they go under the root. | [Container components](https://releases.slint.dev/1.7.0/docs/slint/src/language/concepts/container) | `source` |
| QML: an object has a single default property. Nested objects without a property name go there. | [Object attributes](https://doc.qt.io/qt-6/qtqml-syntax-objectattributes.html) | `source` |
| QML: a delegate reads a model role only through a declared `required property` (Qt for MCUs). | [Non-API differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html) | `source` |
| PropTypes: `PropTypes.element.isRequired` demands exactly one child, checked at run time. | [PropTypes](https://legacy.reactjs.org/docs/typechecking-with-proptypes.html) | `source` |
| GCC 16.2: a slot type that lists allowed kinds and a maximum count rejects wrong content at compile time. | "Measured here" | `measured` |

How the forms compare.

| Form | Flexibility | Control over content | Data to the content |
|---|---|---|---|
| One slot (Slint, QML default property) | Low: one place | None | None |
| Named slots by string (Web Components, Vue) | Several places | None; a wrong name is found late or never | Vue: scoped slots |
| Slots as typed parameters (Compose, Svelte 5) | Several places, optional, with fallback | Presence and signature | Arguments |
| Typed builders and scopes (SwiftUI, Compose scopes) | Several places | Kinds, by type, at compile time | Through the scope |

- The direction of travel is from one implicit slot to slots as typed parameters. Svelte made that move in version 5.
- The most control comes from a type on the slot: SwiftUI's table columns, Compose's scopes.
- A slot with data is the same thing as a row template of a list. Vue's `FancyList` example uses exactly that.

## 6. Parameters and invariants

| Finding | Link | Mark |
|---|---|---|
| Slint: `in` is set by the user of a component, `out` is read by the user, `in-out` both, `private` neither. | [Properties](https://docs.slint.dev/latest/docs/slint/guide/language/coding/properties/) | `source` |
| QML: a `required property` must be set at creation. A violation found statically stops the application from starting. | [Object attributes](https://doc.qt.io/qt-6/qtqml-syntax-objectattributes.html) | `source` |
| Compose: required parameters first, then `modifier`, then optional ones with defaults, then the trailing slot. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| Compose: prefer explicit parameters over implicit inputs. Avoid grab-bag style objects. | [Component guidelines](https://github.com/androidx/androidx/blob/androidx-main/compose/docs/compose-component-api-guidelines.md) | `source` |
| Compose checks values at run time: `require(state.steps >= 0) { "steps should be >= 0" }`. | [Slider.kt](https://github.com/androidx/androidx/blob/androidx-main/compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/Slider.kt) | `source` |
| Vue: `type`, `required`, `default`, `validator`. A failure is a console warning in the development build. | [Props](https://vuejs.org/guide/components/props.html) | `source` |
| Vue 3.4: a validator receives all props as its second argument, so it can check a combination. | [Props](https://vuejs.org/guide/components/props.html) | `source` |
| PropTypes: checked only in development. A custom validator receives all props. React now advises TypeScript. | [PropTypes](https://legacy.reactjs.org/docs/typechecking-with-proptypes.html) | `source` |
| TypeScript: `kind` beside optional `radius?` and `sideLength?` lets the checker accept wrong combinations. | [Narrowing](https://www.typescriptlang.org/docs/handbook/2/narrowing.html) | `source` |
| TypeScript: one interface per `kind` makes each combination a type. `never` checks that all are handled. | [Narrowing](https://www.typescriptlang.org/docs/handbook/2/narrowing.html) | `source` |
| "Make illegal states unrepresentable": a record of optional fields becomes one type per state. | [Minsky, Effective ML](https://blog.janestreet.com/effective-ml-revisited/) | `source` |
| "Parse, don't validate": a parser keeps what it learned in the type. Push the proof to the boundary. | [King, 2019](https://lexi-lambda.github.io/blog/2019/11/05/parse-don-t-validate/) | `source` |
| Vue: props flow one way, down. Mutating a prop warns. | [Props](https://vuejs.org/guide/components/props.html) | `source` |
| Svelte: `$bindable` lets data flow up. Use it sparingly: overuse makes data flow unpredictable. | [$bindable](https://svelte.dev/docs/svelte/$bindable) | `source` |
| React: a component is controlled (driven by props) or uncontrolled (own state). Each state has one owner. | [Sharing state](https://react.dev/learn/sharing-state-between-components) | `source` |
| QML and Slint: assigning a value in code removes the binding of that property. | [Property binding](https://doc.qt.io/qt-6/qtqml-syntax-propertybinding.html), [Reactivity](https://docs.slint.dev/latest/docs/slint/guide/language/concepts/reactivity/) | `source` |
| Slint: `<=>` links two properties so that they always contain the same value. | [Reactivity](https://docs.slint.dev/latest/docs/slint/guide/language/concepts/reactivity/) | `source` |
| QML: a property alias is a direct reference. Bindings are not bi-directional. | [Object attributes](https://doc.qt.io/qt-6/qtqml-syntax-objectattributes.html) | `source` |
| C++26: a contract violated in constant evaluation makes the program ill-formed under an enforcing semantic. | [Contracts](https://en.cppreference.com/w/cpp/language/contracts) | `source` |
| GCC 16.2 rejects `range{10, 0}` at compile time when the constructor has `pre(lo <= hi)`. | "Measured here" | `measured` |
| GCC 16.2 rejects an omitted `required<T>` member in a designated initializer, with our text. | "Measured here" | `measured` |
| GCC 16.2: a `consteval` constructor of a wrapper can check a whole designated parameter set. | "Measured here" | `measured` |
| lvgl26: GCC 16.2 can fold a constant call without its contract inside a larger expression. | project `CLAUDE.md` | `source` |

What this says.

- Other frameworks check combinations at run time, mostly in development builds only. None checks them at compile time.
- TypeScript's unions are the exception: the combination is in the type, so the compiler checks it.
- lvgl26 can do more: the parameters of `lv::use<T>({...})` are constants, so the whole set can be checked by the compiler.
- Two-way binding has two known traps: two copies kept equal by callbacks, and an assignment that silently ends a binding.
- The safe form is an alias: the child's member and the parent's state are one state.

## Not confirmed

- SwiftUI: whether a `ViewModifier` may hold its own `@State`. The page read does not say.
- Solid: whether several `use:` directives on one element are allowed, and in which order they run.
- React: the order in which several effects of one component run.
- Elm: that the runtime compares the subscriptions after each update. Only the signature was confirmed.
- TouchGFX: `handleTickEvent` and `registerTimerWidget` come from forum answers, not from an official page.
- Slint: whether `@children` may appear only once, and whether states exclude each other.
- Slint: the documentation of `changed` callbacks and loop detection was not read, only summarised.
- Slint: that a repeated instance is a heap allocation. The code names a reference-counted `VRc`.
- Slint: the "less than 300 KiB of RAM" figure comes from copies of its README.
- Qt for MCUs: the `ListModel` page returned 404. The read-only statement comes from the differences page.
- Qt for MCUs: that it has no JavaScript engine and no garbage collection comes from a third-party book.
- QML: what happens when several `when` conditions are true.
- GCC 16: the present state of frame elision. The two bug reports were only summarised.
- C++26: that `std::execution::task` was adopted. cppreference lists it; the paper read does not say so.
- libstdc++ 16: whether `std::execution` is shipped, and its code size on a microcontroller.
- Compose, SwiftUI, Svelte: no measured cost per behaviour was found.
- LVGL: the names of the widget-to-subject bind functions were not re-read; the order of handler calls is recalled.
- The frame size not being a constant, and designators not reaching base classes, are recalled.

## What it means for us

The sketches are proposals. Names such as `lv::when`, `lv::each`, `lv::fill` and `lv::run` are placeholders.

### A. Authoring model for dynamic content

A1. Structural nodes in the constant tree.

```cpp
struct playlist {
  lv::list<track, 8> tracks;                 // fixed capacity; reports added, removed, changed rows
  lv::state<bool> empty{true};

  static constexpr auto view = lv::column[
      lv::when(lv::bind(&playlist::empty))[lv::label({.text = "No tracks"})],
      lv::each(&playlist::tracks)[
          lv::use<track_row>({.title = lv::bind(&track::title)})]];
};
```

A2. A builder function that runs again after each change.

```cpp
struct playlist {
  void view(lv::builder& ui) {
    ui.column([&] {
      if (tracks.empty()) ui.label({.text = "No tracks"});
      for (auto& t : tracks) ui.key(t.id).use<track_row>({.title = t.title});
    });
  }
};
```

A3. A static tree with an outlet that code fills by hand.

```cpp
static constexpr auto view = lv::column[lv::column({.id = "rows"})];

void add(const track& t) { rows.push_back(lv::mount<track_row::view>(get<"rows">())); }
```

| | Composability | Compile-time checking | Memory | Dynamic content |
|---|---|---|---|---|
| A1 structural nodes | Nodes nest; a row is any component | Subtree, bindings and capacity are constants | Widgets of live rows, one observer (24 bytes) per node, no diff | Built in; granular row changes |
| A2 re-run builder | Any C++ control flow | Lost: ids, const styles, the tree itself | A retained tree, keys, a description per run | Natural, but needs matching |
| A3 outlet by hand | Whatever the code does | Each subtree is checked, their sync is not | Lowest; nothing added | By hand |

Recommended: A1, built on A3. A structural node is A3 with the bookkeeping written once.

- Main risk: the cost of rows. Each live row is real widgets, and a long list exhausts a 64 KiB heap.
- Second risk: ids and tasks inside a repeated subtree need a per-row home.
- Spike: `lv::when` and `lv::each` over a fixed-capacity model. Insert at the front of 20 rows and prove no other row is rebuilt.
- Measure heap per row with `lv_mem_monitor`, code size on Cortex-M4, and compile instructions on `tests/large_tree.cpp`.

### B. Parameters and invariants

B1. Public members, plus one compile-time check over the whole set.

```cpp
struct stepper {
  lv::required<int> step;
  int min = 0;
  int max = 100;
  lv::state<lv::counter> count;

  static consteval void check(const lv::params<stepper>& p) {
    if (p.min > p.max) lv::fail("stepper: min is above max");
  }
};
```

B2. Types that cannot hold the wrong combination.

```cpp
struct stepper {
  lv::range<int> range{0, 100};        // its constructor: pre(low <= high)
  lv::positive<int> step{1};
  lv::state<lv::counter> count;        // the component keeps it inside range
};

lv::use<stepper>({.range = {0, 10}, .step = 5})
lv::use<stepper>({.range = {10, 0}})   // error: contract predicate is false in constant expression
```

B3. A constructor with contracts instead of public members.

```cpp
struct stepper {
  constexpr stepper(int min, int max, int step) pre(min <= max) pre(step > 0);
};

lv::use<stepper>(0, 10, 5)             // by position: the names are gone
```

| | Composability | Compile-time checking | Memory | Dynamic content |
|---|---|---|---|---|
| B1 check over the set | One hook per component; not shared | Yes for constants, with a message that names members | None | A bound value needs a run-time rule as well |
| B2 checked value types | `lv::range` serves every component | Yes, by type and by `pre` in constant evaluation | None | The type also guards run-time writes |
| B3 constructor | Per component | Yes for constants; GCC 16.2 needs `keep_checked` | None | Same contract at run time |

Recommended: B2 first, then B1 for what is left, with `lv::required<T>`. B3 loses the named parameters.

- Dependent members become one member. What cannot be grouped is checked once, by the compiler.
- A member that changes at run time is guarded where the value enters: the component clamps it, or its setter has `pre`.
- Binding a public state should make the two one state (an alias), not two that are kept equal.
- Main risk: error quality. GCC reports a misspelled designator as "could not convert", without the member's name.
- Note: props are alphabetical, but a plain struct takes designators in declaration order. Generated parameters could keep one rule.
- Spike: generate `lv::params<stepper>` by reflection. Add compile-fail tests for a missing required member, `min > max`, and a wrongly typed binding.

### C. Slots

C1. One slot.

```cpp
struct card {
  const char* title = "";
  static constexpr auto view = lv::column[lv::label({.text = lv::bind(&card::title)}), lv::children];
};

lv::use<card>({.title = "Fan"})[lv::toggle({.id = "fan"}), lv::label({.text = "Auto"})]
```

C2. Named slots as members.

```cpp
struct card {
  lv::slot<> header;
  lv::slot<> body;
  static constexpr auto view = lv::column[lv::outlet(&card::header), lv::outlet(&card::body)];
};

lv::use<card>()[lv::fill(&card::header)[lv::label({.text = "Fan"})],
                lv::fill(&card::body)[lv::toggle({.id = "fan"})]]
```

C3. Named slots with a type: kinds, count, and data for the content.

```cpp
struct dialog {
  lv::slot<> body;
  lv::slot<lv::only<"button">, 3> actions;      // buttons only, three at most
  lv::slot<lv::with<track>> row;                // the content may bind members of track
};

lv::fill(&dialog::actions)[lv::button(...), lv::label(...)]   // error: this slot only takes buttons
lv::fill(&dialog::row)[lv::label({.text = lv::bind(&track::title)})]
```

| | Composability | Compile-time checking | Memory | Dynamic content |
|---|---|---|---|---|
| C1 one slot | Any children, one place | None on the content | None added | The content can hold `when` and `each` |
| C2 named slots | Several places; a slot can be left empty | The slot exists: it is a member pointer | None added | Same |
| C3 typed slots | Same, and a slot type is reusable | Kinds, count, bindings of the content | None added | A slot with data is the row template of `each` |

Recommended: C3, with C1 as its short form (a bare `[...]` fills the one unnamed slot).

- Slot content goes in `[...]`, not in `{...}`: each subtree has its own type, and a designated member has one type.
- Main risk: compile time. The component's view is built once per use, with the content spliced in at each outlet.
- Spike: a card with header, body and actions. Compile-fail tests for a wrong kind, too many children and a missing required slot.
- Measure compile instructions for 50 uses of the card in one tree.

### D. Behaviour over time

D1. Behaviours as constants, piped onto a node.

```cpp
inline constexpr auto pressable = lv::scale_on_press(tok.motion.press) | lv::haptic;   // reusable anywhere

struct stepper {
  lv::state<lv::counter> count;
  void up();
  lv::task<> repeat(lv::view<"button"> plus);   // while held: one step every 100 ms

  static constexpr auto view = lv::row[
      lv::label({.text = lv::bind(&stepper::count)}) | lv::fade_in(tok.motion.enter),
      lv::button({.on_clicked = &stepper::up})[lv::label({.text = "+"})]
          | pressable | lv::run(&stepper::repeat)];
};
```

D2. Member coroutines of the component, found by reflection.

```cpp
struct stepper {
  [[=lv::behavior]] lv::task<> autosave();      // started at mount, destroyed with the component
  [[=lv::behavior]] lv::task<> blink();
};
```

D3. States and transitions as data.

```cpp
static constexpr auto modes = lv::states(&stepper::mode)(
    lv::is<mode::idle>.then({.opacity = tok.opacity.dim}),
    lv::is<mode::active>.then({.opacity = tok.opacity.full}))
    | lv::transition(tok.motion.fade);
```

D4. A setup function that runs once and calls reusable functions.

```cpp
void setup(lv::scope& scope) {                  // at mount
  scope.spawn(autosave());
  use_long_press(scope, get<"plus">(), &stepper::up);
}
```

A reusable behaviour written as a coroutine: one frame, no operator objects.

```cpp
lv::task<> debounce(lv::view<"slider"> slider, lv::time quiet, auto&& then) {
  for (;;) {
    co_await slider.next().value_changed;
    while (co_await lv::first(slider.next().value_changed, lv::sleep(quiet)) == 0) {}
    then();
  }
}
```

| | Composability | Compile-time checking | Memory | Dynamic content |
|---|---|---|---|---|
| D1 piped constants | Several per node, in order; a pipe of two is a behaviour; shared across components | Yes: each behaviour states which widgets it fits | Constants; 12 bytes per handler, a frame per coroutine | Each row gets its own instances, ended with the row |
| D2 member coroutines | Per component, not per node; not shared | Yes: annotation and return type | A frame each, 128 bytes and up | Ended with the component |
| D3 states as data | One table per group of properties | Yes: states, properties, tokens | Bytes, plus running animations | A state per row |
| D4 setup function | Functions call functions | Little: it is ordinary code | Whatever it starts | Runs per instance |

Recommended: D1 as the one mechanism, on top of a lifetime scope owned by every mounted node.

- The scope comes first. Whatever starts in it (task, effect, timer, animation) ends when the widget is deleted.
- Today a task that only sleeps has no owner. A behaviour must be tied to its node when it starts.
- A behaviour is then a constant with an attach step: a plain handler, or a coroutine that needs to wait.
- D2 becomes a short form of D1 on the component's root. D3 becomes one kind of behaviour, added when needed.
- D4 stays possible as the escape hatch, since a scope is an ordinary object.
- This mirrors the two shapes that won elsewhere: attached values (Compose, Svelte) over a scope that cancels (Trio, Kotlin).
- Main risk: memory per instance. One frame per coroutine behaviour, times the rows of a list, comes from LVGL's heap.
- Second risk: order. Two behaviours that handle the same event run in pipe order; that must be stated and tested.
- Spike: a stepper with `fade_in`, `long_press` and `debounce`, in 20 rows of an `each`.
- Measure heap per row and code size on Cortex-M4. Remove a row in mid-animation under AddressSanitizer.
- Add compile-fail tests for a behaviour on the wrong widget kind and for a member that is not a task.

### How the four fit together

- A row template of `each` is a slot with data (A and C).
- A slot's context and a component's members are bound the same way, with `lv::bind(&T::member)` (B and C).
- A structural node, a component and a behaviour all hang on one thing: the lifetime scope of a node (A and D).
- So the first piece to build is the scope, and the first spike the one under D.

### Choices this leaves to the owner

1. Does binding a public state make one state (an alias), or two that are kept equal?
2. Must `each` have a capacity fixed at compile time, or may a list grow on the heap?
3. Are behaviours attached with a pipe, or listed in a `.behaviors` member?
4. Are member coroutines marked with an annotation, or listed by hand?
5. Is slot content named by member pointer (`lv::fill(&card::header)`), or by a quoted name?
