# Coming from Qt Quick

You write QML or use Qt Quick for MCUs, and want the same kind of screen in C++.
This page maps what you know to what lvgl26 has today.

A screen is a tree again, but it is one `constexpr` C++ value, not a document that a runtime reads.
There is no second language, no code generator and no engine: the C++ compiler checks the screen.

## Concepts

| Qt Quick | lvgl26 |
|---|---|
| A `.qml` file with an `Item` tree | A `constexpr` tree: `lv::column({...})[children]`. Properties go in `()`, children in `[]` |
| `Column`, `Row` | `lv::column`, `lv::row`. `lv::group` is a plain box with no padding of its own |
| `anchors.centerIn: parent`, `x`, `y` | `.align = "center"`, `.x`, `.y` with a unit: `.x = 12 * px` |
| `Text { text: ... }` | `lv::label({.text = ...})` |
| `Button { onClicked: ... }` | `lv::button({.on_clicked = &handler})` |
| A property binding `text: speed` | `.text = lv::bind(speed)` |
| A binding with an expression | `lv::computed`, bound the same way |
| A `Q_PROPERTY` of a model object | An `lv::state`. It has a type, and a measure has a unit |
| `Timer { onTriggered: ... }` | A coroutine: `co_await lv::sleep(100 * ms)` in an `lv::task` |
| `Connections { onSomething: ... }` | A coroutine that waits: `co_await button.next().clicked` |
| `NumberAnimation`, `Behavior`, easing | `lv::animate(widget, {.value = {0, 100}}, 300 * ms, lv::ease::in_quad)` |
| `State` and `Transition` | `lv::match` on a state, and animations in a task |
| `Image { source: "a.png" }` | `lv::image_file<a_png>`. The compiler decodes the PNG, and the device needs no decoder |
| `FontLoader` and a font family | `lv::typeface`. The compiler makes the font from a TrueType file |
| `Loader`, `StackView` | `lv::mount<tree>` creates a page and its owner deletes it. See the printer port for a page switcher |
| A Qt Quick Controls style | A theme: `lv::theme({...})` gives each kind of widget its look |
| Colors and sizes kept in one design file | A W3C Design Tokens file, read at compile time. A style takes tokens only |
| A property of the wrong type | A compile error that says what the property takes |
| Work on another thread | `lv::ui_scheduler::post()` on hosted systems |

## What you get that QML does not give

- **Units in the type.** `120 * km / h` is not `120 * px`, and a speed in metres per second that does not fit an integer is refused.
- **Errors at build time.** A wrong property, a misspelled enumerator name or a missing unit stops the build, where QML shows a warning at run time.
- **No runtime.** A tree is built from constants, so the device holds no QML engine and no parser.
- **One licence.** lvgl26 and LVGL are MIT. Nothing in the build is sold.

## Things that are the same

- A screen has a tree, and you change it by changing values.
- Values follow state: change the state and the widgets that are bound to it update.
- A theme sets the look of every widget of a kind, and a node still has the last word.

## Not there yet

- **Components.** A repeated part is a C++ function that returns a piece of tree. Components with their own state, parameters and slots are planned.
- **Lists from a model.** There is no `Repeater` or `ListView` over a model. A list is written as children, and a changed list is mounted again.
- **A style for one part in one state.** The knob of a slider while pressed has no form yet. Themes set a part or a state, not both.
- **Translations.** `qsTr` has no counterpart yet.
- **Qt Design Studio.** There is no visual editor. The workflow is text and a screenshot: `lv::write_png` and the agent tools.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) shows how a look is made once and used by every widget.
- [examples/ports](../examples/README.md) lists the examples that other frameworks inspired.
