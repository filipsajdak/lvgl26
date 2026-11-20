# Code format: how a tree of lvgl26 should look on the page

Research for the owner's request of 2026-11-20: make UI code read like formatted QML, and consider changing the syntax for it.
Every mark is dated 2026-11-20.

- `source`: our code, or a tool's own output or help text read on this machine.
- `measured`: run here: clang-format 21.1.8, 22.1.8 and 23.1.5 (Homebrew), GCC 16.2 with `-fsyntax-only`, instructions of `cc1plus` from `/usr/bin/time -l`.
- `source` for another framework: its page, read on 2026-11-20 and linked in the row.
- `recalled`: from memory, with no page read. Check those rows before relying on them.

## Short answers

| Question | Answer | Mark |
|---|---|---|
| Is format enforced today? | No. There is no `.clang-format`, no CI step and no hook. The examples follow a Google-like style by hand | `source` |
| Can clang-format give the owner's form with today's syntax? | No. It reads `[...]` as a subscript: children align under the bracket, and the end stays `})[track_row]]];` | `measured` |
| What is the lever? | A trailing comma. A braced list ending in `,` gets one property per line; without it, a list stays on one line while it fits. clang-format does not document this | `measured` with versions 21, 22 and 23 |
| What does a syntax change buy? | Children as arguments, `lv::group({...}, child, child)`, let clang-format lay out the whole tree as nested blocks | `measured` |
| What does it cost? | Two overloads of the factories and one of `lv::each`, about 15 lines. 0.1 G more instructions on a 47.6 G file. `[]` keeps working | `measured` |
| Is clang-format safe on our C++26? | Yes, with `SortIncludes: Never`. `^^`, `[: :]`, `pre`, `post`, `#embed` and `template for` survive, and the result compiles | `measured` |
| Can properties be written in any order? | Not with designated initializers: GCC rejects them out of declaration order. A named-argument form can, at a cost in editor support | `measured` |
| Should a QML-like file format come in? | Not now. It loses completion, types and messages at the user's line, for a look that C++ can come close to | reasoned |

## 1. The target

### The owner's form as rules

The owner's example implies these rules:

1. One property per line once a block does not fit on one line.
2. A nested braced value, such as `.style`, opens a block: `.style = {` ends its line.
3. The `=` signs of one block are aligned.
4. The children open on their own bracket, and each child starts a line.
5. Every closing bracket stands on its own line, at the indent of the line that opened it.
6. A lambda's body is on lines of its own.
7. Four spaces per level.

Rules 1, 3, 6 and 7 are clang-format options. Rules 2, 4 and 5 are where today's syntax and clang-format part ways (section 2).

### What makes QML read well

The same screen, as QML would write it; written for this note, not taken from a page.

```qml
Column {
    width: 320; height: 240
    spacing: Tokens.space.gap
    Label {
        visible: !hasTracks
        color: Tokens.color.quiet
        text: "No tracks"
    }
    Column {
        width: parent.width
        Repeater { model: tracks; delegate: TrackRow {} }
    }
}
```

- The type is the head of the block, and the block holds both properties and children.
- One property per line, `name: value`, with no punctuation after it.
- Closing is one `}` per object. There is no `)`, `]` or `,` soup.
- Short objects may stay on one line: `Text { text: "x" }`.
- `qmlformat` "automatically formats QML files according to the QML Coding Conventions"; its examples nest objects as brace blocks. Its page states no rule of one property per line. `source`, [qmlformat](https://doc.qt.io/qt-6/qtqml-tooling-qmlformat.html)

Slint has the same shape: an element is a name followed by braces that hold its properties and children. `source`, [Slint files](https://docs.slint.dev/latest/docs/slint/guide/language/coding/file/)
Its formatter is `slint-lsp format -i <path>`. `source`, [Slint tooling](https://docs.slint.dev/latest/docs/slint/guide/tooling/manual-setup/)

SwiftUI, Jetpack Compose and Flutter are code, like ours, and each has its own way to the same look.

| Framework | Children are written | Properties are written | What its formatter does |
|---|---|---|---|
| SwiftUI | In a trailing closure: `VStack { Text("a"); Text("b") }`. `recalled`: the page was not read | As modifiers after the closing brace: `.padding(8)`. `recalled` | Not checked |
| Jetpack Compose | "Composables usually take a `content` composable lambda", written `Column { ... }`. `source`, [Compose layouts](https://developer.android.com/develop/ui/compose/layouts/basics) | As named arguments | The Kotlin conventions say "Group multiple closely related arguments on the same line" and give no one-per-line rule. ktfmt was not read. `source`, [Kotlin conventions](https://kotlinlang.org/docs/coding-conventions.html) |
| Flutter | As a named argument, `children: [ ... ]`. `recalled` | As named arguments | "`dart format` adds or removes trailing commas based on whether the construct splits across multiple lines". An opt-in `trailing_commas: preserve` forces a split. `source`, [dart format](https://dart.dev/tools/dart-format), [dart_style FAQ](https://github.com/dart-lang/dart_style/wiki/FAQ) |
| Dear ImGui style C++ | As statements between `Begin` and `End` | As calls | Ordinary code layout |

What C++ can have of this:

- One property per line, aligned, with the type at the head: yes, through designated initializers and a trailing comma.
- Properties and children inside one block: only as two blocks in one call, `({...}, children)`. A braced list cannot hold children of different types.
- No closing punctuation: no. A designated initializer needs `{}`, and a call needs `()`. The best layout puts each closer on its own line, so it reads as an end marker.
- Free property order: not with designated initializers (section 3).

Flutter is the closest relative: its children are arguments, and the trailing comma marks a list that is split. Dart's formatter now writes the comma itself; clang-format leaves it to us. That is what this note proposes for lvgl26.

## 2. Formatting alone, with today's syntax

### What is there

- No `.clang-format` in the repository. Nothing checks format in `ci.yml`, in a hook or in a test. `source`
- The examples follow a Google-like style at 120 columns, with two spaces per level. `source`
- The Mac has clang-format 21, 22 and 23. The editor CI image installs clang 21 (`ARG CLANG=21` in `.github/ci-image/editor/Dockerfile`); the main image has no Clang. `source`
- All three versions gave the same output for every test in this note. `measured`

### Is it safe on our code

- `^^::`, `[:R:]`, `pre(x > 0)`, `post(r: r > 1)`, `contract_assert`, `#embed` inside an initializer and `template for` keep their meaning. `post(r: ...)` becomes `post(r : ...)`, which is valid. `measured`
- Over the 148 files of `examples/`, the only changes to anything but white space come from `SortIncludes`. With `SortIncludes: Never` none remain. `measured`
- `examples/lists/main.cpp`, reformatted, still compiles. `measured`

### The options that matter

Each was tried on the tree of `examples/lists/main.cpp`.

| Option | Effect on our trees | Mark |
|---|---|---|
| A trailing comma in a braced list | One element per line, and the list opens with `({` and closes with `})` on its own line. The options page documents no such rule: it is observed behaviour | `measured` with versions 21, 22 and 23 |
| No trailing comma | The list stays on one line while it fits; when it does not, its elements align under the first one | `measured` |
| `AlignConsecutiveAssignments: Consecutive` | Aligns the `=` of designated initializers inside one block | `measured` |
| `AlignAfterOpenBracket: BlockIndent` | Arguments of a call open and close like a block, `(` at the end of a line and `)` on its own line. It does not apply to `[...]` | `measured` |
| `BinPackArguments: false` | One argument per line once a call breaks | `measured` |
| `AllowShortLambdasOnASingleLine: None` | A lambda's body on its own lines, as the owner wrote it | `measured` |
| `BreakAfterOpenBracketBracedList`, `BreakBeforeCloseBracketBracedList` (version 23) | No visible effect on designated initializers | `measured` |
| `Cpp11BracedListStyle: false` | Spaces inside braces, `{ .x = 1 }`. With trailing commas, nested lists are left exactly as written: nothing is formatted, so nothing is enforced | `measured` |
| `ColumnLimit` | 100 or 120. 100 breaks more blocks, which with trailing commas changes little | `measured` |

Two things no option changed:

- A nested block that breaks always comes out as `.style =` and then `{` on the next line, indented four more. This happens in versions 21, 22 and 23, with every option above. `measured`
- `[...]` is a subscript. Children align under the bracket, three spaces in, and the closing stays `})[track_row]]];`. `measured`

Once, clang-format returned a variant unchanged instead of formatting it: `lv::group({...})(...)` under `BlockIndent`. Its layout search seems to give up on deep nesting. A check must therefore compare output, not trust that it ran. `measured`

### The best result with today's syntax

The tree of the lists example, written with trailing commas, after clang-format with the configuration below and the small pass described next:

```cpp
constexpr auto ui = lv::group({
    .flex_flow = "column",
    .size      = {320 * px, 240 * px},
    .style = {
        .bg_color = tok.color.screen,
        .bg_opa   = tok.opacity.opaque,
        .pad_all  = tok.space.edge,
        .pad_gap  = tok.space.gap,
    },
})[lv::label({
       .hidden = lv::bind(has_tracks),
       .style = {
           .text_color = tok.color.quiet,
       },
       .text = "No tracks",
   }),
   lv::group(
       {
           .flex_flow = "column",
           .size      = {100 * pct, lv::size_content},
           .style = {
               .pad_gap = tok.space.gap,
           },
       }
   )[lv::each(
       tracks, &track::id,
       {
           .selected = lv::bind(current),
       }
   )[track_row]]];
```

The first block is the owner's form. The children are not: they hang under `[`, and `]]]` closes three levels at once.

Without trailing commas, a real file comes out cramped. This is the shell of the printer port as written today, under the same configuration: `measured`

```cpp
constexpr auto ui = lv::group(
    {.size = {window_width * px, window_height * px},
     .style =
         {.bg_color    = tok.window.background,
          .bg_opa      = tok.solid,
          .clip_corner = true,
          .radius      = tok.window.radius}}
)[lv::group({.id = "main", .size = {page_width * px, 100 * pct}, .x = sidebar_width * px}),
  ...
```

So a formatter alone, on code as written today, makes it worse. The trailing comma has to come with it.

### A pass after clang-format

A prototype of 40 lines of Python joins `.style =` and the `{` below it into `.style = {`. It moves the block left by the extra indent. `measured`

- It changes only white space; the test compares the file without white space before and after.
- It runs after clang-format, so a check runs both and compares with the file.
- It cannot fix the children under `[`: that is clang-format's reading of a subscript, and a pass would have to re-indent whole sub-trees.
- With a second rule it can join `lv::group(` and `{`. The children then sit to the right of the closing `},`, which reads worse than leaving it.

Another formatter was not tried. Uncrustify has an option for a newline between `=` and `{` (`recalled`). It is not installed here, and a change of formatter would lose the editors' built-in clang-format.

### When a thing stays on one line

The trailing comma is the user's choice, and so it is the rule for short things:

- A list without a trailing comma stays on one line while it fits: `lv::label({.text = "One"})`, `.style = {.text_color = tok.color.quiet}`. `measured`
- A list with one is broken, whatever its length.
- The rule for the docs: a block with one or two short properties stays on one line. A block with three or more, or with a nested block, ends with a comma.

This is QML's own habit, `Text { text: "x" }` on one line and larger objects as blocks.

## 3. Syntax that formats better

Each was written into a copy of `examples/lists/main.cpp`, formatted, and compiled with `-fsyntax-only` against `include/` of `main` at `8d984c7`. Instructions are the least of three runs of the whole file.

### A. Children as arguments: `lv::group({...}, child, child)`

Two overloads take the properties first and the children after them. A third lets `lv::each` take its row template last. They forward to today's `[]`, which stays.

```cpp
template <class D = void, tree_kid... Kids>
  requires(sizeof...(Kids) > 0)
constexpr auto operator()(const lazy_props<W, D>& p, const Kids&... kids) const {
  return make_node<W, D>(p)[kids...];
}
```

The lists screen, formatted with the configuration of section 5 and the pass:

```cpp
constexpr auto ui = lv::group(
    {
        .flex_flow = "column",
        .size      = {320 * px, 240 * px},
        .style = {
            .bg_color = tok.color.screen,
            .bg_opa   = tok.opacity.opaque,
            .pad_all  = tok.space.edge,
            .pad_gap  = tok.space.gap,
        },
    },
    lv::label({
        .hidden = lv::bind(has_tracks),
        .style  = {.text_color = tok.color.quiet},
        .text   = "No tracks",
    }),
    lv::group(
        {
            .flex_flow = "column",
            .size      = {100 * pct, lv::size_content},
            .style     = {.pad_gap = tok.space.gap},
        },
        lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)
    )
);
```

| Check | Result | Mark |
|---|---|---|
| Compiles | Yes, the whole example | `measured` |
| Compile time | 47.60 G today, 47.68 G in this form: 0.2 % | `measured` |
| clang-format | Lays it out natively: properties as the first block, each child on its own line, one `)` per level | `measured` |
| Difference from the owner's form | The first `{` stands on its own line under `lv::group(`. `.style = {` is not aligned with its siblings | `measured` |
| A wrong call | `lv::label({...}, 5)` gives GCC's "no matching function" with the candidates. It needs a deleted fallback with a message, and a compile-fail test | reasoned |
| Editor stubs | The two overloads must be declared in `tools/editor/stubs`, and used in `tests/editor/api_uses.cpp` | `source` (CLAUDE.md) |
| Reference data | Unchanged: it lists properties, not factories | `source` |
| Existing code | Keeps compiling; it can move file by file | `measured` |

### B. Children in a second call: `lv::group({...})(child, child)`

clang-format treats the second `(` like `[`: the children align under it, and `BlockIndent` gave up on the file once. It formats no better than today. `measured`

### C. Properties and children in one braced block

`lv::group{{...props...}, child, child}` or `.children = {...}` would need a class template per widget with deduction from the children. A braced list holds one type, so children of several types cannot sit in one `{}`. This is a redesign of every factory, for a layout that A already gives. Not tried.

### D. Properties in any order

Designated initializers must follow declaration order. GCC 16.2 says: "designator order for field `...::hidden` does not match declaration order". `measured`

- Today the generated props are in alphabetical order, so the order is the one a reader can guess. `source` (`site/status.html`)
- Another fixed order (id, layout, size, content, state, style, handlers) reads better. But the user must learn it, and the message names only one field.
- Free order needs another carrier. A sketch outside the library, `label("text"_p = "No tracks", "hidden"_p = true)`, compiles. Each named value finds its member by reflection; an unknown name fails a `static_assert`. `measured`
- Its costs: no completion inside a string, a new form beside the designated one, and each property a template of its own. Its compile time was not measured on real props.
- Recommendation: keep alphabetical order. Teach "properties are in alphabetical order" once, and let the editor's completion list them in that order.

### E. A deep tree split into named constants

```cpp
// Shown while the list is empty.
constexpr auto empty_note = lv::label({
    .hidden = lv::bind(has_tracks),
    .style  = {.text_color = tok.color.quiet},
    .text   = "No tracks",
});

// One row per track; the selected one is checked.
constexpr auto track_list = lv::group(
    {
        .flex_flow = "column",
        .size      = {100 * pct, lv::size_content},
        .style     = {.pad_gap = tok.space.gap},
    },
    lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)
);

// Groups: boxes without the theme's look, laid out as columns.
constexpr auto ui = lv::group(
    {
        .flex_flow = "column",
        .size      = {320 * px, 240 * px},
        .style = {
            .bg_color = tok.color.screen,
            .bg_opa   = tok.opacity.opaque,
            .pad_all  = tok.space.edge,
            .pad_gap  = tok.space.gap,
        },
    },
    empty_note, track_list
);
```

- It compiles, and costs 47.70 G against 47.60 G. `measured`
- Each piece gets a name and a comment, as a QML file gets a component. No block is deeper than two levels.
- It works today, with `[]` too: `track_row` in the lists example is already such a piece. `source`
- The docs should teach it as the main way to keep a screen readable: a screen of pieces, each under a page of code.

### F. Looks, handlers, units

- `.style` blocks shrink most when the look moves to the token file: `lv::look<"queue.card">` holds a whole style in one name. See [less-boilerplate.md](less-boilerplate.md), lever 1. `source`
- A handler in a constexpr tree is a function or a lambda without captures (`CLAUDE.md`). With `AllowShortLambdasOnASingleLine: None` its body gets lines of its own. `measured`
- `320 * px` is already as short as a unit literal would be; a user-defined literal (`320_px`) would add a second spelling. Not proposed.

## 4. A declarative file

A QML-like or JSON file, embedded with `#embed` and read at compile time, would give QML's look exactly. The parts exist: `lv::tokens` reads JSON in the compiler, and `lv::runtime` builds a tree from a document at run time. `source`

What it would cost:

- Editor support. Our stubs give completion and checks in C++. A new file format needs its own language server, as `qmlls` and `slint-lsp` are for theirs. `source` ([lsp.md](lsp.md))
- Messages. A mistake becomes a compile-time error from a reader, computed text that names a line of the document. Today GCC points at the user's line.
- Expressions. A binding, a handler, a lambda, `100 * pct`, a token by name: each needs a syntax and a reader. QML has JavaScript for this; we would have names that refer back to C++.
- Compile time. Every file that includes the screen reads and parses the document in the constant evaluator. The token files cost 5 to 7 G each ([compile-diagnosis.md](compile-diagnosis.md)).

It suits people who design screens in a tool and hand over a file; that is `lv::runtime`'s place. As the way to write code by hand, it trades the compiler's help for punctuation. Not recommended now.

## 5. Enforcement and migration

### A configuration

```yaml
BasedOnStyle: Google
ColumnLimit: 100
IndentWidth: 4
ContinuationIndentWidth: 4
SortIncludes: Never
ReflowComments: false
AlignConsecutiveAssignments: Consecutive
AlignAfterOpenBracket: BlockIndent
BinPackArguments: false
AllowShortLambdasOnASingleLine: None
```

- Two spaces per level instead of four keeps the rest of the code as it is; the trees then indent by two and four.
- `AlignConsecutiveAssignments` also aligns ordinary assignments in function bodies.

### Checks

- A `.clang-format` at the root of the repository. One more in the project template or the `write-ui` skill, so users get the same layout.
- A CI step for `examples/`: clang-format, then the pass, compared with each file. It runs in the editor image, which has clang 21.
- A rule a script can check: a braced list that spans lines ends with a comma.
- A test of the lever itself. The trailing-comma behaviour is undocumented, so the check formats a small file with and without commas and compares with the expected layout. A newer clang-format that changes it then fails the check, not the examples.
- `InsertTrailingCommas`, which would add the commas, is for JavaScript only. `summary`, [clang-format options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- The docs need no step of their own. `tools/docs.py sync` copies their code from the examples, so formatting the examples formats the docs.
- `include/` follows later or never: a reader of the docs does not see its code. Even the base style would change 1020 of its 18,959 lines. `measured`

### Size of the one-time change

| Scope | Lines | Rewritten by the configuration above | Mark |
|---|---|---|---|
| `examples/`, 148 files | 6706 | 3870, in 142 files | `measured` |
| The same, Google style at 120 columns with two spaces | 6706 | 1566, in 133 files | `measured` |
| Docs | 61 snippet blocks | Rewritten by `tools/docs.py sync` from the examples | `source` |
| The lists example in form A | 84 lines | 123 lines | `measured` |

The trailing commas are not added by clang-format. They are written by hand once per block, or by a small script that adds one to every braced list clang-format broke.

### Order for the documentation programme

1. Decide the format and the syntax: the decisions below.
2. Add form A, with its fallback message, compile-fail test and stubs.
3. Reformat `examples/` in one pull request with no other change, and add the CI check.
4. Then rewrite the docs, so that every snippet is written once.

## 6. Properties in an order a person reads

The owner asked on 2026-11-20 whether properties can follow the order a person reads: the id first, then size and position, style, content and handlers. Editor completion and compile-time checks must stay. Measured on 2026-11-20 unless a row says otherwise.

### Short answers

| Question | Answer | Mark |
|---|---|---|
| Can C++ take designators in any order? | Not in C++26. P3405R0, "Out-of-order designated initializers" (2024), proposes it; it was not found adopted | `summary` |
| Can the generated struct have another order? | Yes. The order is one sort in `props.hpp`; a prototype sorts by group, then by name, and the lists example compiles | `measured` |
| What does it cost the compiler? | Nothing measurable on the Mac: 47.72 G against 47.73 G for the lists example. The nightly probes were not run | `measured` |
| Can a wrong order get a better message? | Not from lvgl26. GCC rejects the list before any of our code runs, and names one field | `measured` |
| How much code moves? | 195 of 1407 property lists in examples, tests and docs, by a script | `measured` |

### What C++ and the compilers say

- C++20 made designated initializers follow declaration order (P0329R4). [P0329R4](https://isocpp.org/files/papers/p0329r4.pdf) `summary`
- [P3405R0](https://open-std.org/JTC1/SC22/WG21/docs/papers/2024/p3405r0.html) proposes out-of-order designators, `{.y = 1, .x = 2}`. It is a first revision of October 2024; no later one was found. `summary`
- A WG21 liaison message of 2020 called a change of the order rule incompatible, needing a strong rationale. [liaison list](https://lists.isocpp.org/liaison/2020/08/0159.php) `summary`
- [P2287](https://open-std.org/JTC1/SC22/WG21/docs/papers/2025/p2287r4.html) adds designators for base classes and keeps the order rule. `summary`

GCC 16.2 on `lv::label({.text = "x", .hidden = true})`, verbatim: `measured`

```
error: designator order for field 'lv::detail::props_of<lv::detail::fixed<6>{"label"}, void>::type::hidden' does not match declaration order in 'lv::detail::lazy_props<lv::detail::fixed<6>{"label"}, void>' {aka 'const lv::detail::props_of<lv::detail::fixed<6>{"label"}, void>::type'}
```

Clang 23 on the same shape only warns, and names both fields: `measured`

```
warning: ISO C++ requires field designators to be specified in declaration order; field 'text' will be initialized after field 'hidden' [-Wreorder-init-list]
```

- The error comes from GCC's reading of the braced list, before any code of ours sees the values. No `static_assert`, constructor or concept can replace it.
- The editor shows Clang's warning at the user's line, and completion lists the members in declaration order. So the order a user must follow is the order completion offers.

### Option O1: alphabetical, as today

```cpp
constexpr auto empty_note = lv::label({
  .hidden = lv::bind(has_tracks),
  .style  = {.text_color = tok.color.quiet},
  .text   = "No tracks",
});
```

- A reader can guess it without a list. The style falls between the state and the content.
- Handlers sort among properties: `.on_clicked` comes before `.size`.

### Option O2: four groups by name, alphabetical inside each

The id, then the properties, then the styles, then the handlers. The group follows from the name alone: `id`, `style` and `style_...`, `on_...`, and everything else.

```cpp
constexpr auto empty_note = lv::label({
  .hidden = lv::bind(has_tracks),
  .text   = "No tracks",
  .style  = {.text_color = tok.color.quiet},
});

constexpr auto track_row = lv::button(
  {
    .id            = "row",
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style         = {...},
    .style_checked = {.bg_color = tok.color.selected},
    .on_clicked    = &pick,
  },
  ...
);
```

| Check | Result | Mark |
|---|---|---|
| The change | One comparison, by group and then by name. It replaces the sorts of `props.hpp` and the merge of a widget's own setters into the base's | `measured` (prototype) |
| Compiles | The lists example in this order compiles | `measured` |
| Compile time | 47.72 G in O2, 47.73 G in O1, the same file otherwise; inside the noise of about 0.2 % | `measured` |
| Stable across LVGL versions | Yes: the groups come from the names, which reflection reads from LVGL's setters and events | `source` |
| Reference data | Written again: `tools/reference/reference.cpp` prints the members in struct order | `source` |
| Editor stubs | `tools/editor/reference_stubs.py` sorts members by name (line 145); it takes the same key | `source` |
| Existing code | 195 of 1407 property lists change, most because a style comes before `.text` or a handler before `.size`. A script moves designators within a list | `measured` |
| The rule to teach | "id, properties, styles, handlers; each group alphabetical" | reasoned |

### Option O3: five groups, the widget's own properties first

The id, then what the widget is for (its own setters: `text`, `value`, `src`). Then what every widget has (`align`, `size`, `hidden`), then styles, then handlers.

```cpp
constexpr auto empty_note = lv::label({
  .text   = "No tracks",
  .hidden = lv::bind(has_tracks),
  .style  = {.text_color = tok.color.quiet},
});
```

- Reflection knows the difference: own setters are `lv_label_set_*`, the base's are `lv_obj_set_*`. `source` (`own_members` in `props.hpp`)
- The reader does not: whether `checked` belongs to the button or to every widget is LVGL's choice. The order then needs completion to be found.
- Not prototyped. Its cost would be that of O2: one more group in the same comparison.

### Option O4: a fixed human order from a table

A hand-written order per property (id, size, position, layout, content, style, handlers) reads best. It breaks the rule against hand-written per-property code, and every new LVGL setter needs a place in it. Not recommended.

### Option O5: free order with named values

`lv::label("text"_p = "No tracks", "hidden"_p = lv::bind(has_tracks))` takes any order: section 3 D shows a sketch that compiles. It loses completion inside the string and adds a second way to write properties. Not recommended, unless P3405 is adopted and a future GCC accepts any order, which would make every option here unnecessary.

### Recommendation

O2. It gives the owner's order for the parts that matter most to a reader: the id first, content before style, handlers last. The rule fits in one sentence, the compiler pays nothing, and the change is a sort key and a one-time script. O3 is the next step if the owner wants content before layout too.

## The owner's decisions, 2026-11-20

The owner read the candidates below and decided.

| Question | Decision |
|---|---|
| Children | As call arguments: `lv::group({props}, child, child)`. `[...]` keeps compiling; examples and pages use the call form |
| Layout switch | A trailing comma, with clang-format and the small pass after it |
| Indent | Two spaces, so examples, docs and `include/` share one indent. `include/` is not reformatted in this work |
| Readability | Named pieces of at most about three levels per constant, taught as a rule with an id |
| Property order | Alphabetical stays for now. More research into an order a person reads comes first |
| A declarative file | No |

The work follows in two pull requests: the call form in the library, then the format and its check over `examples/`.

## The candidates the owner chose from

1. **Children as arguments (form A).** Recommended. It is the one change that lets a formatter keep the tree as blocks, and `[]` stays for one-liners.
2. **Trailing comma as the layout switch.** Recommended. A block with a trailing comma breaks; a short one without stays on one line.
3. **Four spaces or two.** Four matches the owner's example; two keeps the library's code unchanged. Recommended: four in `examples/` and the docs, two in `include/` until it is reformatted.
4. **The pass after clang-format for `.style = {`.** Recommended, as a 40-line tool in `tools/` run by the check. Without it, nested blocks show `.style =` and a `{` below.
5. **Property order.** Keep alphabetical. Free order costs completion and a second form.
6. **Named pieces as the main way to stay readable.** Recommended for the docs: no tree deeper than about three levels in one constant.
7. **A declarative file.** Not now.

The same screen in each candidate:

Today:

```cpp
constexpr auto ui = lv::group({.flex_flow = "column", .size = {320 * px, 240 * px},
                               .style = {.bg_color = tok.color.screen, .bg_opa = tok.opacity.opaque,
                                         .pad_all = tok.space.edge, .pad_gap = tok.space.gap}})[
    lv::label({.hidden = lv::bind(has_tracks), .style = {.text_color = tok.color.quiet}, .text = "No tracks"}),
    lv::group({.flex_flow = "column", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.gap}})[
        lv::each(tracks, &track::id, {.selected = lv::bind(current)})[track_row]]];
```

Today's syntax, formatted (decisions 2 to 4): the block in "The best result with today's syntax" above.

Form A, formatted (decisions 1 to 4): the block in section 3 A.

Form A with named pieces (decisions 1 to 6): the block in section 3 E.

The owner's hand form survives only with the formatter off for the region (`// clang-format off`), or with a formatter of our own. Neither is recommended: the first is not enforced, and the second is a C++ parser to maintain.
