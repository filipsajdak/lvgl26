# Code format: how a tree of lvgl26 should look on the page

Research for the owner's request of 2026-11-20: make UI code read like formatted QML, and consider changing the syntax for it.
Every mark is dated 2026-11-20.

- `source`: our code, or a tool's own output or help text read on this machine.
- `measured`: run here: clang-format 21.1.8, 22.1.8 and 23.1.5 (Homebrew), GCC 16.2 with `-fsyntax-only`, instructions of `cc1plus` from `/usr/bin/time -l`.
- `recalled`: from memory. The web tools failed in this session, so no page of another framework was read. Check those rows before relying on them.

## Short answers

| Question | Answer | Mark |
|---|---|---|
| Is format enforced today? | No. There is no `.clang-format`, no CI step and no hook. The examples follow a Google-like style by hand | `source` |
| Can clang-format give the owner's form with today's syntax? | No. It reads `[...]` as a subscript: children align under the bracket, and the end stays `})[track_row]]];` | `measured` |
| What is the lever? | A trailing comma. A braced list ending in `,` gets one property per line; without it, a list stays on one line while it fits | `measured` |
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

The same screen, as QML would write it. `recalled`

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
- `qmlformat` keeps this layout; it is the reference for "formatted QML". `recalled`

Slint has the same shape: `Text { text: "x"; }`, properties and children in one block. Its language server formats it. `recalled`; that `slint-lsp` formats is `source` in [lsp.md](lsp.md).

SwiftUI, Jetpack Compose and Flutter are code, like ours, and each has its own way to the same look. `recalled`

| Framework | Children are written | Properties are written | What its formatter does |
|---|---|---|---|
| SwiftUI | In a trailing closure: `VStack { Text("a"); Text("b") }` | As modifiers after the closing brace: `.padding(8)` | One child per line |
| Jetpack Compose | In a trailing lambda, `content`: `Column(modifier = ...) { Text("a") }` | As named arguments | ktfmt and the Kotlin conventions put each argument on its own line when a call does not fit |
| Flutter | As a named argument, `children: [ ... ]` | As named arguments | Before Dart 3.7 a trailing comma forced one argument per line; since 3.7 the formatter decides and adds the commas itself |
| Dear ImGui style C++ | As statements between `Begin` and `End` | As calls | Ordinary code layout |

What C++ can have of this:

- One property per line, aligned, with the type at the head: yes, through designated initializers and a trailing comma.
- Properties and children inside one block: only as two blocks in one call, `({...}, children)`. A braced list cannot hold children of different types.
- No closing punctuation: no. A designated initializer needs `{}`, and a call needs `()`. The best layout puts each closer on its own line, so it reads as an end marker.
- Free property order: not with designated initializers (section 3).

Flutter's old rule is the closest relative: its children are arguments, and a trailing comma decides the layout. That is what this note proposes for lvgl26. `recalled`

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
| A trailing comma in a braced list | One element per line, and the list opens with `({` and closes with `})` on its own line | `measured` |
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

## Decisions for the owner

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
