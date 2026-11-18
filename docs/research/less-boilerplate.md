# Less boilerplate: looks, repetition by reflection, texts and parts

Research for the owner's review of pull request 100, the reworked Slint printer port, written 2026-11-18.

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-18 unless its row says otherwise.

- `source`: our code, LVGL's code in `build/_src/lvgl-v9.6.0` and the v10 pin, or the Slint files at commit `f3c4537`.
- `measured`: compiled, run or counted in this session; "Measured here" says how.
- `summary` and `recalled`: as the README says.

The owner marked five places of the port "Boilerplate!!!".
His aim, in his words: "rebuild the same ui without writing lot of boilerplate and by using highlevel concepts that are checked in compiletime".
He named the main cause: "the most code repetition (boilerplate) is for setting design tokens".
He asked: "we can pass all tokens and each element will take only the ones tokens that it is interested in".

This note builds on decisions 24 to 27 of `shown-theme-format.md` (pull request 99) and on `node-storage.md`.
Those decide that an element names its look, `.look = "queue.card"`, and that group entries match style properties by name.
This note sketches how that works, what else removes code, and counts the result against Slint.

The sketches live outside the repository, in a scratch directory.
This note quotes what they showed.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| Can a look be found by its path and turned into a style by member names? | Yes: one constant style per path, with fonts, enumerators and state layers | Sketch 1, run |
| Does a look's text style reach a label without a look of its own? | Yes: LVGL inherits text color, font and alignment from the parent | Sketch 5, run; `lv_style.c` |
| Do size and layout belong in a look? | They can: width, height, flex, padding and alignment are LVGL style properties | Reference data; Sketch 5 |
| Can four tiles come from the four enumerators of `subpage`? | Yes: text, icon, look and handler all from the enumerator's name | Sketch 2 and 5, run |
| Can detail lines come from the members of `job`? | Yes: annotations with a caption, read by reflection in GCC 16.2 | Sketch 2 and 5, run |
| Can tubes come from a constant array of records? | Yes, with ids made from the record's name | Sketch 2, run |
| Can `"100%\n75%\n50%"` be a list? | Yes, joined by the compiler into one string in read-only memory | Sketch 3, run |
| Can the texts come from a strings file, checked for every language? | Yes; a missing text in one language stops the build | Sketch 4, run |
| Does a language switch allocate? | The heap stays flat, but each label copies its text, as LVGL's text binding does | Sketch 4, run; `lv_label.c` |
| Can a sub-tree from another file be part of the page's tree? | Yes, today, as a constant in a header; no owner to release | Sketches 2 and 5 nest constant trees |
| How much shorter is the home page? | 83 words of code against 294 today and 294 in Slint; with the token entries it adds, about 288 | "The count" |
| Do looks remove words or move them? | Mostly move: the cut is in repetition, inherited text and fonts. Code loses about 900 words, the token file gains about 600 | "The count", "Ranked levers" |

## The count

This table is the heart of the note.
Each row counts the same piece of the UI in the Slint original, in pull request 100, and in the target syntax of this note.

How it was counted (`measured`):

- A line is a line that is not blank and not only a comment.
- A word is an identifier, a number or a string literal; punctuation is not counted.
- Slint: `demos/printerdemo/ui` at `f3c4537`.
  Pull request 100: `origin/refactor/printer-port-highlevel`.
- The target is the syntax of "The five snippets" below, written into files and counted by the same script.
- "Token entries" is what the target moves into the token file.
  It is design data, not code, but it is not free.

| Piece | What is counted in each column | Slint lines / words | PR 100 lines / words | Target lines / words | Token entries the target adds | Target words with its entries |
|---|---|---|---|---|---|---|
| (a) detail lines of a job | Slint: `PrintQueueDetailsLabel`, `PrintDetails`, its `if` and `states`. PR 100: `detail_line` and the details group | 84 / 149 | 10 / 94 | 3 / 27 | 9 | 72 |
| (b) spin box, the part | Slint: `spinbox.slint`. PR 100: `spin_box`, `fewer`, `more`, `one_fewer`, `one_more` | 42 / 94 | 16 / 132 | 10 / 97 | 16 | 177 |
| (b) spin box, the call | The call and its state | 6 / 11 | 2 / 21 | 2 / 17 | 0 | 17 |
| (c) the tile's label | Slint: `label := Text`. PR 100: the `lv::label` of `tile` | 6 / 13 | 3 / 18 | 1 / 7 | 3 | 22 |
| (d) the four tiles | Slint: the `for action in [...]` block. PR 100: the four calls and the four `open_*` functions | 43 / 97 | 6 / 56 | 1 / 12 | 14 | 82 |
| (d) the tile, the part | Slint: `ActionButton`. PR 100: `tile` | 48 / 113 | 14 / 104 | 5 / 36 | 19 | 131 |
| (e) the four tubes | Slint: `ink-levels`. PR 100: the four calls, `levels`, four spawns, four `get` | 6 / 22 | 9 / 86 | 4 / 44 | 12 | 104 |
| Home page, whole | Slint: `ActionButton` and `HomePage` without the four sub-pages. PR 100: `home_page.cpp`, the queue's mount, `open_*` | 118 / 294 | 49 / 294 | 17 / 83 | 41 | 288 |
| Ink page, whole | Slint: `ink_page.slint` without imports. PR 100 and target: `ink_page.cpp` | 77 / 210 | 59 / 383 | 30 / 174 | 25 | 299 |

What the table shows:

- In lines, pull request 100 is already shorter than Slint: a C++ line holds more than a Slint line.
  Words are the fair measure.
- In words, pull request 100 matches Slint on the home page and loses on the ink page (383 against 210).
- In code, the target needs a third of Slint's words on the home page, and four fifths on the ink page.
- With its token entries, the target is about even with Slint on the home page and shorter on the ink page.
- For one use, a look moves words into the token file; it removes them only where it is used again.
- So (b), (c) and the tile are longer with their entries than in pull request 100: each is used once in the count.
- The spin box serves Copy and Scan, and its look serves both; the tile look serves four tiles.
- The palettes are not counted in any column: Slint's `common.slint` and the port's semantic tokens.
- Slint stays shorter in (b) at the call site and in (e): see "Where Slint stays shorter".
- The token entries are counted from the sketch's token file where it has them (tile, spin box, details), and estimated for the rest.
- An entry costs 5 words in the DTCG form (`"radius": {"$type": "dimension", "$value": "24px"}`): 166 words for 34 entries of `home` and `tile`. `measured`

### The design tokens named in code

| | PR 100 | Target |
|---|---|---|
| `tok.` references in the port | 197, on 133 lines (`grep -o "tok\.[a-z_.]*"` over `*.cpp *.hpp`) | About 10: the 8 motion tokens and a few computed values; reasoned |
| `type(tok.text.X)` font lines | 19 lines, 133 words | None: a look's `text_font` entry draws the font |
| Words inside `.style = {...}` blocks | 632 of the port's 4,731 words, in 72 blocks | 2 words per block: `.look = "x.y"` |
| Words in `.size`, `.flex_*`, `.x`, `.y`, `.align` | 614 words in 167 properties | The fixed ones move into looks; computed and percent ones stay |

Measured with a script over the 23 C++ files of the port, without `icons.hpp` and `fonts.hpp`, comments removed.
The most used: `.text_color` 26 times, `.bg_opa` 26 (25 of them `tok.solid`), `.bg_color` 23, `.radius` 17, `.pad_gap` 13.

## Measured here

GCC 16.2 on macOS arm64, LVGL v9.6.0 built by the `host-v96` preset of the worktree.
Every sketch was compiled with the full `host-v96` flag line of `compile_commands.json`, with `-Werror`, and linked against LVGL.
They ran with LVGL's test display (`lv_test_display_create`).
No sketch was built under a sanitizer or against the v10 pin.

| Sketch | What it showed | Result |
|---|---|---|
| 1 `looks.cpp` | `look<"tile"> \| look<"tile.print">` on a button, `look<"tile.label">` on a label, `look<"status">` with a nested `checked` group | Width 162, height 192, radius 24, colors and gradient as in the file; label white, centered, Barlow 18 px drawn by the compiler; status blue when checked, gray when not |
| 1 | A look path with a typo, `"tile.lable"` | `constexpr message: lv: no token group 'lable' for this look.` |
| 1 | A token entry that fits no style property, `"colour"` | `constexpr message: lv: the look entry 'colour' is no style property.` |
| 1 | Compile cost: four looks against the same styles written out | 125.85 G and 125.00 G instructions, two runs each: about 0.2 G per look path |
| 2 `repeat.cpp` | `repeat<enumerators_but<subpage, subpage::none>>(lv::row(...), tile)` | Four buttons, captions and icons by the enumerator's name; a click on the third sets `subpage::copy` |
| 2 | Detail lines from `[[=caption{"Job Owner"}]]` on members of `job` | Four lines in an `lv::each` row: "Job Owner: info@example.com", "Pages: 6", "Size: 143kb", "Submitted: 11:41" |
| 2 | Tubes from a constant array of records, ids from the records | `get<"yellow">` and `get<"cyan">` find them; texts "90%" and "50%" made by the compiler |
| 2 | An annotation whose type holds a `std::string_view` | `annotation does not have structural type`: a caption must hold a fixed array of characters |
| 3 `options.cpp` | `.options = options({"100%", "75%", "50%"})` and the options of `enum class quality` | LVGL reads "100%\n75%\n50%" and "best\nmedium\ndraft" |
| 3 | `.selected = lv::bind(state<quality>)` | Refused today: "this property cannot be bound to a state of that type" |
| 3 | `.selected = lv::bind(state<pick>)`, `struct pick { std::uint32_t value; }` | Follows both ways: the state selects, a pick by the user sets the state |
| 4 `tr.cpp` | Texts from `strings.json`, `en` and `de`, a `language` state, one computed per key | "Printer / Print" then "Drucker / Drucken"; heap +0 bytes after one and after 100 switches |
| 4 | Is the label's text the table's pointer? | No: the label holds a copy |
| 4 | A language that lacks a text; a key that no language has | `lv: language 'de' has another number of texts.` and `lv: no text 'home.prnt' in the strings file.` |
| 5 `home.cpp` | Home tiles from the enum, with looks; label without a look | Tiles 162 x 192 in two rows of two, colors per tile; each label 22 px high, the font of the tile's look |
| 5 | The name label of the spin box, without a look, against a bare label outside it | `1d293d` and Barlow 20 px high, from the look `spinbox`; the bare label `212121` and 16 px, LVGL's default |
| 5 | The tile label's white | Not evidence: LVGL's default theme already makes text in a button white |
| 5 | `spin_box<copies>("Copies")`, handlers inside, state as template argument | Three clicks on plus show 4; heap +0 bytes |
| 5 | Details with `look<"queue.details">` and a `checked` layer | `max_height` 504 and `pad_top` 8 read back in the checked state |
| 5 | `lv::group({.style = look<"home.tiles">})` | The tiles did not wrap: the group's default size of 100 percent is a local style and beats the look's width |
| 5 | The same node made bare without that default size | The tiles wrap into two rows |
| 2 | `[[=caption{...}]]`, `annotations_of`, `enumerators_of`, `identifier_of`, `members_of` of a namespace, splice of a member found by name | All used in `repeat.cpp` and `home.cpp`, built with the full flags |

Not shown by the sketches: the height of the opened details.
A plain tree with today's `.style` and `.style_checked` also reads 0 there in the headless test.
So the test, not the look, is the open point.

## Findings

| Finding | Where | Mark |
|---|---|---|
| GCC 16.2 reads annotations (P3394): `annotations_of`, `constant_of`, a splice of the value | Probe and Sketch 2 | `measured` |
| An annotation must have a structural type; `std::string_view` is not one | Sketch 2 | `measured` |
| A lambda with a template parameter, `[]<subpage P> {...}`, can be called once per element of a constant range and its results spliced into `node[...]` | Sketches 2 and 5 | `measured` |
| `std::define_static_string` makes a joined option list or a computed id a constant the tree can keep | Sketches 2 and 3 | `measured` |
| A captureless handler can name a template argument: `[] { active_subpage = [:E:]; }` | Sketch 2 | `measured` |
| LVGL inherits `text_color`, `text_opa`, `text_font`, `text_align`, letter and line spacing, decoration and `base_dir` | `src/misc/lv_style.c`, `LV_STYLE_PROP_FLAG_INHERITABLE` | `source` |
| `image_recolor` is not inherited: an icon in a button needs its own look | same | `source` |
| `width`, `height`, `x`, `y`, `align`, `min_*`, `max_*`, `pad_*`, `layout`, `flex_*` and `grid_*` are style properties | `docs/reference/lvgl-v9.6.0.json`, `style` | `source` |
| `layout` is typed as a number; flex needs `layout = flex` as well as `flex_flow`, as `lv_obj_set_flex_flow` sets both | Sketch 5 | `measured` |
| `lv_dropdown_set_options` copies the options; `lv_dropdown_set_options_static` keeps the pointer | `lv_dropdown.h` doc comments | `source` |
| `lv_roller_set_options` documents its parameter as "a string with '\n' separated options" | `lv_roller.h` | `source` |
| The option properties are `dropdown.options`, `dropdown.options_static` and `roller.options`; `buttonmatrix.map` and `keyboard.map` take arrays | reference data | `source` |
| LVGL has a translation module, `LV_USE_TRANSLATION`, 0 in `lv_conf_template.h`, in v9.6 and the v10 pin | `include/lvgl/core/lv_translation.h` | `source` |
| `lv_translation_add_static` keeps the caller's arrays but allocates a list node per pack | `src/others/translation/lv_translation.c` | `source` |
| `lv_translation_set_language` reallocates the language name and sends an event to every widget | same | `source` |
| A translated text is found by `strcmp` over all tags of the pack, at every change | same, `get_translation` | `source` |
| `lv_label_set_translation_tag` duplicates the tag on the heap and copies the text into the label | `lv_label.c` | `source` |
| Slint's `@tr("...")` marks a text for gettext; translations come from `.po` files, at run time or bundled at build | Slint documentation | `recalled` |
| DTCG has no type for an enumerator or a percentage; lvgl26 reads `"$type": "string"`, which DTCG does not define | `include/lv/tokens.hpp`, `token_type_of` | `source` |

## Lever 1: named looks carry everything visual

### Before and after

```cpp
// PR 100: the card of the queue, its frame
lv::group({.checkable = true, .flex_flow = "column", .size = {card_width * px, lv::size_content},
           .state_trickle = true,
           .style = {.bg_color = tok.queue.card, .bg_opa = tok.solid, .border_color = tok.queue.card_border,
                     .border_width = tok.control.thin, .pad_all = tok.space.card, .pad_gap = tok.space.gap,
                     .radius = tok.queue.radius}})

// Target
lv::group({.checkable = true, .look = "queue.card", .state_trickle = true})
```

The token file holds the group, by style property names, with aliases to the semantic tokens:

```json
"queue": {
  "card": {
    "layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "column"},
    "width": {"$type": "dimension", "$value": "275px"},
    "bg_color": {"$value": "{color.card}"}, "bg_opa": {"$value": "{solid}"},
    "border_color": {"$value": "{color.card_border}"}, "border_width": {"$value": "{line.thin}"},
    "pad_all": {"$value": "{space.card}"}, "pad_row": {"$value": "{space.gap}"}, "radius": {"$value": "{radius.queue}"}
  }
}
```

### How it works (Sketch 1)

- The project registers its design once, as `lv::cfg` is registered: the token value and the typeface.
- `look<"queue.card">` is a variable template: one constant style per path, made where it is first used.
- The resolver walks the path through the generated token structs, then matches each entry to the style member of the same name.
- A typography entry on `text_font` goes through the typeface, so the font is drawn where the look is used.
- A string entry on an enum property is an enumerator by name, as `.align = "center"` is in a tree.
- A string entry on `layout` is an enumerator of `lv_layout_t`: the property is typed as a number.
- A nested group named after a state or a part becomes an `lv::on` layer: `queue.details.checked`.
- Any other nested group is a look of its own, not part of its parent.
- A wrong path and an entry that fits no property are compile errors that name them.

### Where `.look` is resolved

The sketch wrote `.style = look<"queue.card">`, which needs no change in `include/lv`.
The decided spelling `.look = "queue.card"` needs one thing the sketch could not test.

- A factory receives its properties as a function parameter, which is not a constant there.
- So the string cannot become a template argument inside `lv::group(...)`.
- It can at mount: `apply_node` reads each node as a constant (`constexpr auto a = N.p.[:m:]`), as it reads `.text` today.
- So `.look` is a `const char*` member of the props, read in `apply_node`, and resolved to `look<path>` there. `reasoned`
- The cost is one member more in `kept<W>`, beside about 160 others.
  `tests/large_tree.cpp` must measure it first. `reasoned`

### Variants: tile and tile.print

The four tiles share a frame and differ in color.
Three ways to say that, all compiled except the last two rows:

| Option | Call site | Cost |
|---|---|---|
| Two looks merged with `\|` | `.style = look<"tile"> \| look<"tile.print">` | Compiled; long |
| A list in one string, as CSS classes | `.look = "tile tile.print"` | One parse more at mount; reasoned |
| A path takes its parents' entries | `.look = "tile.print"` | Wrong for `tile.inner`, which must not take the tile's size; reasoned |

### Text by inheritance, not by cascade of names

A label inside a tile needs no look: the tile's look sets `text_color` and `text_font`, and LVGL inherits them.
Sketch 5 shows a bare `lv::label({.text = ...})` in the spin box with the look's color `1d293d` and its font.
The same label outside reads LVGL's default, `212121` and 16 px.

A cascade by name, where a label takes `tile.label` because it sits in a `tile`, would save the same words.
It would also be implicit: the look of a label would depend on where a part is used.
LVGL's inheritance is explicit in the parent's look, costs nothing, and is how LVGL themes already work (`docs/themes.md`).
A text role look (`.look = "label.secondary"`) remains for the label that differs from its parent.

### Size and layout in a look

Every layout setting is a style property in LVGL, so decision 25 already covers it.

- Fixed sizes, padding, gaps, flex flow and alignment move into looks.
- Percent sizes cannot: a DTCG dimension has no percent.
  They stay in the tree, or lvgl26 reads `"100%"` as an extension.
- Positions computed from other sizes (`(page_width - column_width) * px`) stay in the tree: a token file cannot compute.
- `lv::group` sets a size of 100 percent as a local style, which beats a look.
  Its default must give way to a look.

### What the compiler gets, and what it costs

- Facts: every look path, every entry and every font is checked at compile time with a message that names it.
- Run time: a look is the same constant `lv_style_t` that `.style` makes today, shared between equal looks.
  No heap.
  `source`, `include/lv/style.hpp`
- Compile time: about 0.2 G instructions per look path in Sketch 1, a file of 125 G. `measured`
- The resolver scans the 139 style members by name for each entry; a name table made once would cut that. `reasoned`
- `large_tree`: no cost while its nodes name no look, if the scan sits behind the path template. `reasoned`
- Modes (decision 12): `design` holds one token set per mode, and `look<path>` yields one style per mode, swapped in place. `reasoned`

### Smallest slice

`lv::design` registration, `lv::look<"path">` as a style value, and the `lv::group` default yielding to a look.
`.look = "..."` as a node member follows, after `large_tree` has measured it.

## Lever 2: repetition from what the compiler knows

### (d) one tile per enumerator

```cpp
// PR 100
tile<icons::print>("Print", tok.tile.print, &open_print), tile<icons::scan>("Scan", tok.tile.scan, &open_scan),
tile<icons::copy>("Copy", tok.tile.copy, &open_copy), tile<icons::usb>("USB", tok.tile.usb, &open_usb)]];
// and in state.hpp: open_print, open_scan, open_copy, open_usb

// Target
lv::group({.look = "home.tiles"})[lv::for_each<subpage, lv::but<subpage::none>>(tile)],
```

The tile takes the enumerator and derives the rest from its name: icon `icons::print`, look `tile.print`, text `home.print`.
Its handler is `[] { active_subpage = Page; }`, so the four `open_*` functions go.
Sketch 2 ran exactly this, with the enumerator as a reflection and the icon and the text found by name in a namespace.

### (a) one line per annotated member

```cpp
struct job {
  job_id id;
  const char* title;
  [[=lv::caption{"queue.owner"}]] const char* owner;
  [[=lv::caption{"queue.pages"}]] lv::counter pages;
  [[=lv::caption{"queue.size"}]] const char* size;
  [[=lv::caption{"queue.submitted"}]] const char* submitted;
  // ...
};

lv::group({.look = "queue.details"})[lv::for_each_field<job>([]<lv::field F> {
  return lv::row[lv::label({.look = "queue.name", .text = F.caption}), lv::label({.look = "queue.value", .text = F.text})];
})]
```

`F.text` is `lv::bind(&job::owner)` for a text and `lv::bind("{}", &job::pages)` for a number.
The caption is a key of the strings table, so the details are translated with the rest.
A table of `{caption, member}` pairs in a `constexpr` array works too, and keeps the row type free of UI words.

### (e) one element per record or per enumerator

```cpp
enum class ink { yellow, cyan, magenta, black };
constexpr int level[] = {90, 50, 80, 10};
lv::group({.look = "ink.tubes"})[lv::for_each<ink>(tube)]
```

The tube's id is the enumerator's name, its color the look `ink.level.yellow`, its text "90%" made by the compiler.
PR 100 writes the level twice, as `90` in `levels` and as `"90%"` in the call; the target writes it once.
A record array with names works too, but each name must have the same fixed length to share a type.

### How it works, and what it costs

- `lv::for_each<R>(f)` calls `f.template operator()<R[i]>()` for each element and splices the results into the parent's `[...]`.
- Sketch 2 spelled it `repeat<R>(parent, f)`, because `node::operator[]` takes nodes, not a pack in one value.
- Letting `operator[]` flatten such a value is one overload in `include/lv/tree.hpp`. `reasoned`
- The result is the same tree as written by hand: no run-time cost, no heap, no list.
  `measured`: the same widgets came out.
- The scans run where the template is instantiated: `enumerators_of`, `annotations_of` and `members_of` of one namespace.
  They are lazy.
- A row of `lv::each` with reflected lines worked in Sketch 2: the bindings to row fields are found as before.
- Interaction: `lv::shown` decides which tree; `lv::for_each` writes several fixed ones.
  `lv::each` is for rows that change at run time.

### Smallest slice

`lv::for_each<E>` over an enum, with `lv::but<...>`, and the flattening in `operator[]`.
Captions and `for_each_field` second.

## Lever 3: shorter leaves

- With inherited text, a label is `lv::label({.text = ...})`.
- A text-first overload, `lv::label("Print")` or `lv::label(lv::tr<"home.print">)`, would cut it to the text. `reasoned`
- The overload takes one argument that is a text or a text binding; the props form stays.
- A part that takes children by name is a slot (`components.md`, section 5); it removes no words in the printer.
- Layout from the look: covered in lever 1.

## Lever 4: components with their state and handlers inside

```cpp
// Target, the part
template <lv::dependency auto& Count>
constexpr auto spin_box(lv::text name) {
  return lv::row({.look = "spinbox"})[
      lv::label({.text = name}),
      lv::button({.look = "spinbox.button", .on_clicked = [] { Count.update(&one_fewer); }})[lv::image({.look = "spinbox.icon", .src = lv::image_file<icons::minus>})],
      lv::label({.look = "spinbox.value", .text = lv::bind("{}", Count)}),
      lv::button({.look = "spinbox.button", .on_clicked = [] { Count.update(&one_more); }})[lv::image({.look = "spinbox.icon", .src = lv::image_file<icons::plus>})]];
}

// Target, the call
spin_box<options::count>(lv::tr<"job.copies">)
```

- The state is a template argument, as decision 19 wants: states are not injected.
- The handlers are captureless lambdas that name it; `fewer` and `more` go from `job_options`.
- Sketch 5 ran this shape: three clicks on plus showed 4, with no heap.
- The look group `spinbox` holds the parts: `button`, `icon`, `value`.
- This needs no library change today, except `lv::text` as the type of a text parameter. `reasoned`
- Slint's `SpinBox` declares `minimum` and `maximum` as properties; ours keeps them in `one_fewer` and `one_more`.

## Lever 5: option lists

```cpp
// PR 100
choice("Size", "100%\n75%\n50%", fonts.label)

// Target
choice(lv::tr<"job.size">, lv::options{"100%", "75%", "50%"})
choice(lv::tr<"job.quality">, lv::options_of<quality>, lv::bind(chosen_quality))
```

- Sketch 3 joined the list at compile time with `std::define_static_string`; an empty option or a line break in one is refused.
- Which properties take such lists: the setters whose doc comment says "'\n' separated options".
  The angle units are found the same way.
- A constant list is in read-only memory, so the library can call `lv_dropdown_set_options_static`: no copy on LVGL's heap. `reasoned`
- `.selected` cannot follow an enum state today; it can follow a named index (Sketch 3).
- With options made from an enum, `.selected = lv::bind(enum_state)` is safe: index and enumerator agree by construction. `reasoned`
- The options of an enum need their texts from the strings table, not the enumerator names, to be translated.

## Lever 6: texts from the design

### What the sketch did

- `strings.json` holds one object per language, with nested groups: `{"en": {"home": {"print": "Print"}}, "de": {...}}`.
- The compiler reads it, checks that every language has every key, and keeps a table of pointers in read-only memory.
- `lv::tr<"home.print">` names a key; a key that does not exist is a compile error.
- A `language` state selects the column; each bound label follows it.
- The heap stayed flat over 100 switches, but each label holds a copy of its text.

### Without the copy

- A text that is a pointer into a constant table can be shown with `lv_label_set_text_static`, as constant text is today.
- Then a switch changes a pointer and redraws: no copy.
  That needs a static-text binding, the same need as decided for formatted text (decision 15). `reasoned`
- One computed per key costs a subject and an observer per bound key; one observer per label on the language state would cost less. `reasoned`

### LVGL's own module

- `lv_translation` is off by default, allocates a tag copy per label and a text copy per change, and looks texts up by `strcmp`.
- It checks nothing at compile time: a missing tag shows the tag.
- Our table can still feed it, as a static pack, for C code that uses `lv_tr`. `reasoned`

### Where the texts come from

- DTCG has no string type for a text, and a token file is not where translators work.
- A separate `strings.json`, one object per language or one file per language, keeps the design file for values.
- A Figma export of text layers, or a gettext `.po` file turned into that JSON by a tool, can write it. `reasoned`
- Slint's `@tr("Print")` keeps the English text in the code, and the text is the key. `recalled`
- Ours names a key, so a change of the English text needs no change of the code.
- (g) In the original the names are code: `name: @tr("Print")` in `home_page.slint`.
- With `lv::tr_of<"home", Page>` they come from the strings file, by the enumerator's name, and a translator can reach them.

## Lever 7: a sub-tree from another file in the page's tree

```cpp
// PR 100, home_page.cpp
lv::owned<lv::obj> column = mount_queue_view(page.get());
column.release();  // the queue lives and dies with the page

// Target: queue_view.hpp exports the tree, and the page's tree holds it
constexpr auto ui = lv::group[ ..., queue_view];
```

- A constant tree in a header nests into another tree today.
  Sketches 2 and 5 nest constant trees, also inside `lv::each`.
- Its widgets are built and owned by the page's mount; `release()` and `mount_queue_view` go.
- The cost: the queue's tree is compiled in the home page's file.
  The port split files for compile time, so this must be measured. `reasoned`
- A page function arm (decision 3 of `shown-theme-format.md`) remains the way for pages that `lv::shown` switches.
  It uses the storage it declares (`node-storage.md`, decisions 1 and 7).
- A `lv::part` with a declared storage size would let a file keep its tree out of the page's file.
  It is the page function of a sub-tree. `reasoned`
- The owner's review asked why `release()` is called; this removes the question.

## The five snippets in the target syntax

(a) the detail lines of a job:

```cpp
lv::group({.look = "queue.details"})[lv::for_each_field<job>([]<lv::field F> {
  return lv::row[lv::label({.look = "queue.name", .text = F.caption}), lv::label({.look = "queue.value", .text = F.text})];
})]
```

(b) the spin box: the part in lever 4; its call is `spin_box<options::count>(lv::tr<"job.copies">)`.

(c) the tile's label:

```cpp
lv::label({.text = lv::tr_of<"home", Page>})
```

(d) the four tiles:

```cpp
lv::group({.look = "home.tiles"})[lv::for_each<subpage, lv::but<subpage::none>>(tile)]
```

(e) the four tubes:

```cpp
lv::group({.look = "ink.tubes"})[lv::for_each<ink>(tube)]
```

### What the token file must contain

The groups below are those of the sketch's file, in one line per entry, with literal values.
The port's file would alias its semantic tokens instead, as `"{label.primary}"`.
The `ink` groups were not in the sketch: they are written here. `reasoned`

For (a), the details of a job:

```json
"queue": {
  "details": {"layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "column"},
              "max_height": {"$type": "dimension", "$value": "0px"},
              "checked": {"max_height": {"$type": "dimension", "$value": "504px"}, "pad_top": {"$type": "dimension", "$value": "8px"}}},
  "name": {"text_color": {"$type": "color", "$value": "#122f7b"}, "text_font": {"$value": "{text.detail}"}},
  "value": {"text_color": {"$type": "color", "$value": "#c1c3ca"}, "text_font": {"$value": "{text.detail}"}}
}
```

For (b), the spin box and its parts:

```json
"spinbox": {
  "layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "row"},
  "flex_cross_place": {"$type": "string", "$value": "center"}, "pad_column": {"$type": "dimension", "$value": "12px"},
  "height": {"$type": "dimension", "$value": "40px"},
  "text_color": {"$type": "color", "$value": "#1d293d"}, "text_font": {"$value": "{text.body}"},
  "button": {"width": {"$type": "dimension", "$value": "40px"}, "height": {"$type": "dimension", "$value": "40px"},
             "radius": {"$type": "dimension", "$value": "14px"}, "bg_color": {"$type": "color", "$value": "#277aff"},
             "shadow_width": {"$type": "dimension", "$value": "0px"},
             "pressed": {"bg_color": {"$type": "color", "$value": "#1d293d"}}},
  "icon": {"image_recolor": {"$type": "color", "$value": "#ffffff"}, "image_recolor_opa": {"$type": "number", "$value": 1}},
  "value": {"width": {"$type": "dimension", "$value": "40px"}}
}
```

For (c) and (d), the tiles: the label's entries are the last two of `tile`.

```json
"home": {"tiles": {"layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "row_wrap"},
                   "pad_row": {"$type": "dimension", "$value": "16px"}, "pad_column": {"$type": "dimension", "$value": "16px"},
                   "width": {"$type": "dimension", "$value": "340px"}, "height": {"$type": "dimension", "$value": "400px"}}},
"tile": {
  "width": {"$type": "dimension", "$value": "162px"}, "height": {"$type": "dimension", "$value": "192px"},
  "radius": {"$type": "dimension", "$value": "24px"}, "bg_opa": {"$type": "number", "$value": 1},
  "bg_grad_dir": {"$type": "string", "$value": "ver"}, "shadow_width": {"$type": "dimension", "$value": "0px"},
  "layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "column"},
  "flex_main_place": {"$type": "string", "$value": "center"}, "flex_cross_place": {"$type": "string", "$value": "center"},
  "pad_row": {"$type": "dimension", "$value": "10px"},
  "text_color": {"$type": "color", "$value": "#ffffff"}, "text_font": {"$value": "{text.label}"},
  "inner": {"width": {"$type": "dimension", "$value": "64px"}, "height": {"$type": "dimension", "$value": "64px"},
            "radius": {"$type": "dimension", "$value": "16px"}, "bg_color": {"$type": "color", "$value": "#ffffff"},
            "bg_opa": {"$type": "number", "$value": 0.2}},
  "print": {"bg_color": {"$type": "color", "$value": "#2b7fff"}, "bg_grad_color": {"$type": "color", "$value": "#155dfc"}},
  "scan": {"bg_color": {"$type": "color", "$value": "#00bc7d"}, "bg_grad_color": {"$type": "color", "$value": "#009966"}},
  "copy": {"bg_color": {"$type": "color", "$value": "#ff2056"}, "bg_grad_color": {"$type": "color", "$value": "#fd3d6d"}},
  "usb": {"bg_color": {"$type": "color", "$value": "#ff6900"}, "bg_grad_color": {"$type": "color", "$value": "#f54900"}}
}
```

For (e), the tubes:

```json
"ink": {
  "tubes": {"layout": {"$type": "string", "$value": "flex"}, "flex_flow": {"$type": "string", "$value": "row"},
            "pad_column": {"$type": "dimension", "$value": "50px"}, "pad_left": {"$type": "dimension", "$value": "89px"},
            "pad_top": {"$type": "dimension", "$value": "73px"}, "y": {"$type": "dimension", "$value": "86px"}},
  "level": {"yellow": {"bg_color": {"$value": "{color.yellow}"}}, "cyan": {"bg_color": {"$value": "{color.cyan}"}},
            "magenta": {"bg_color": {"$value": "{color.magenta}"}}, "black": {"bg_color": {"$value": "{color.black}"}}}
}
```

A group-level `"$type"` would drop one word per entry; DTCG allows it on a group.
A short form `"radius": "24px"` would drop three more, but DTCG does not allow it.

### The home page, whole

```cpp
// The home page: four actions, and the print queue beside them.

#include "pages.hpp"
#include "parts.hpp"
#include "queue_view.hpp"

namespace printer {
namespace {

// One of the four actions. Everything visual is in the looks "tile" and "tile.<page>".
constexpr auto tile = []<subpage Page> {
  return lv::button({.look = lv::looks<"tile", Page>, .on_clicked = [] { active_subpage = Page; }})[
      lv::group({.look = "tile.inner"})[lv::image({.look = "tile.icon", .src = lv::image_of<^^icons, Page>})],
      lv::label({.text = lv::tr_of<"home", Page>})];
};

constexpr auto ui = lv::group[
    lv::label({.look = "home.title", .text = lv::tr<"home.title">}),
    lv::group({.look = "home.tiles"})[lv::for_each<subpage, lv::but<subpage::none>>(tile)],
    queue_view];

}  // namespace

lv::owned<lv::obj> mount_home(lv::obj parent) { return mount_page<ui>(parent); }

}  // namespace printer
```

### The ink page, whole

```cpp
// The Ink page: four tubes that fill to the level of each ink.

#include "pages.hpp"
#include "parts.hpp"

namespace printer {
namespace {

enum class ink { yellow, cyan, magenta, black };
constexpr int level[] = {90, 50, 80, 10};  // percent, as PrinterState.ink-levels in the original

// One tube: its outline, the ink that rises in it, and the level written on a pill.
constexpr auto tube = []<ink Ink> {
  return lv::group({.look = "ink.tube"})[
      lv::group({.id = lv::name_of<Ink>, .look = "ink.fill"})[lv::group({.look = lv::looks<"ink.level", Ink>})],
      lv::group({.look = "ink.outline"}),
      lv::group({.look = "ink.pill"})[lv::label({.text = lv::text<"{}%", level[std::to_underlying(Ink)]>})]];
};

constexpr auto ui = lv::group({.look = "page"})[
    plain_header(lv::tr<"ink.title">),
    lv::group({.look = "ink.tubes"})[lv::for_each<ink>(tube)]];

// The ink rises to its level, a moment after the page appears.
lv::task<> fill(lv::obj ink, int percent) {
  co_await lv::animate(ink, {.height = {0 * px, tube_height * percent / 100 * px}}, tok.motion.ink_time, tok.motion.ink);
}

lv::task<> rise(lv::tree_view<ui> page) {
  co_await lv::sleep(tok.motion.ink_delay);
  template for (constexpr ink i : lv::enumerators<ink>) lv::spawn(fill(page.get<lv::name_of<i>>(), level[std::to_underlying(i)]), page.root());
}

}  // namespace

lv::owned<lv::obj> mount_ink(lv::obj parent) {
  auto built = lv::mount<ui>(parent);
  if (!built) return {};
  lv::spawn(rise(built->view()), built->root());
  return lv::owned<lv::obj>{built->detach()};
}

}  // namespace printer
```

Names in the target that do not exist yet: `.look`, `lv::looks`, `lv::for_each`, `lv::but`, `lv::for_each_field`, `lv::field`, `lv::caption`, `lv::tr`, `lv::tr_of`, `lv::image_of`, `lv::name_of`, `lv::text`, `lv::options`, `lv::options_of`, `lv::enumerators`, `lv::tree_view`.
`lv::tree_view` is `lv::row_view` renamed for both uses, as decision 9 of `shown-theme-format.md` has it.

## Where Slint stays shorter

- The call of a part: `SpinBox { value: 1; minimum: 1; label: @tr("Copies"); }` has 11 words; ours 17 with its state.
- Slint gives a property a default in the component; a C++ state needs its own line and type.
- The ink levels: Slint writes `{ color: #00c1e2, level: 60% }` as data with a color in it.
  We split the color into the token file.
- Layout: `HorizontalLayout { spacing: 50px; width: 75%; }` computes from the parent.
  A token file cannot compute, so sizes become fixed numbers.
- Text from numbers: `color-info.level * 100 + "%"` is an expression; ours needs `lv::text<"{}%", ...>` or a binding.
- Slint pays at run time for this: an interpreter or generated code that evaluates bindings.
  Ours resolves looks, lists and texts in the compiler.

## Each lever: cost, risk and slice

`tests/large_tree.cpp` compiles under a limit of 6,500,000 operations and needs about 5,780,000.
A lever risks it only if it changes what every node holds or copies.

| Lever | Run time: bytes and heap | Where the scan runs; risk to `large_tree` | Interaction with decided features | Smallest slice |
|---|---|---|---|---|
| 1 Looks | The same constant `lv_style_t` as `.style`, in read-only memory, shared by equal looks; no heap | In `look<path>`, a variable template, on first use of a path. A `.look` member adds one of about 160 members to `kept<W>`: measure first | Modes (decision 12) give one style per mode; decision 25 maps entries; `lv::on` layers come from nested groups | `lv::design` registration and `lv::look<"path">` as a style value |
| 2 Repetition | None: the same widgets as written by hand | `enumerators_of`, `annotations_of`, `members_of` of one namespace, in the part's instantiation only; none for `large_tree` | `lv::each` for rows that change; `lv::shown` for which tree; injected handlers (decision 19) see the element as a constant | `lv::for_each<E>` and the flattening in `operator[]` |
| 3 Shorter leaves | None; inheritance is LVGL's own lookup at draw time | None | Formatted text (decision 16) is the text form | A text-first overload of `lv::label` |
| 4 Parts with handlers inside | A function per handler; no heap | None | States stay template arguments (decision 19); `lv::tree_handler` accepts the lambdas | None: works today |
| 5 Option lists | The joined list in read-only memory; with `options_static`, no copy on LVGL's heap | `define_static_string` at the call | An enum state for `.selected` needs a binding to an index | `lv::options{...}` |
| 6 Texts | A table of pointers in read-only memory; a subject per bound key; a text copy per label per switch until a static-text binding exists | Reading `strings.json` once per program file that names a key | The static-text binding is the same need as formatted text (decision 15) | `lv::tr<"key">` with keys checked |
| 7 Sub-trees | None beyond the widgets; no owner object | The sub-tree compiles in the page's file | Page function arms (decision 3) and declared storage (node-storage decisions 1 and 7) stay for switched pages | Move the queue view's tree into a header |

## Ranked levers

Wins are across the whole port: 864 lines and 4,731 words today.
The 72 style blocks hold 161 entries, and only 52 of the blocks differ: a look said once replaces the repeats. `measured`
They overlap: lever 1 includes the words that inherited text saves.

| Rank | Lever | Win across the port | Library cost | Risk |
|---|---|---|---|---|
| 1 | Looks with inherited text (1 and 3) | Code: about 490 of 632 style-block words, 133 font-line words and half the 614 layout words, about 900 words, a fifth of the port; about 187 of 197 `tok.` go. Token file: about 120 entries, 600 words, come back | A resolver, the design registration, `.look` in `apply_node`, the group default | Compile time per look; `.look` in `kept<W>` costs `large_tree` |
| 2 | Repetition by reflection (2) | Tiles, tubes, two detail lists, the settings rows: about 25 lines and 250 words | `for_each`, `but`, `for_each_field`, flattening in `operator[]` | Errors come from inside a lambda; a record array needs fixed-length names |
| 3 | Sub-trees in the page's tree (7) | About 8 lines, and the `release()` the owner questioned | None: works today | Compile time moves into the page's file |
| 4 | Parts that own their handlers (4) | `fewer`, `more` and the function pointers: about 40 words per part kind | `lv::text` as a parameter type | None found |
| 5 | Option lists (5) | No words; ten `"\n"` strings become lists; no copy of options on LVGL's heap | `lv::options`, `options_static`, enum selection | Translated options need the strings table |
| 6 | Texts from a strings file (6) | Costs one or two words per text; derived keys (`tr_of`) give some back | Strings reader, `lv::tr`, a static-text binding | A text of the code moves into a file a translator edits |

## What was compiled, and what was reasoned

| Claim | Compiled and run | Compiled only | Reasoned |
|---|---|---|---|
| Looks by path, by member name, with fonts, enumerators, layout and state layers | Sketches 1 and 5 | | |
| Error messages for a wrong path and an unknown entry | | Sketch 1 | |
| `.look = "..."` as a node member read in `apply_node` | | | Yes |
| Several looks in one string; variants | `look<a> \| look<b>` | | The string form |
| Text inherited from a parent's look | Sketch 5 | | |
| Looks per mode | | | Yes |
| Tiles from enumerators; handler naming the enumerator | Sketches 2 and 5 | | |
| Detail lines from annotations inside `lv::each` | Sketches 2 and 5 | | |
| Tubes from a record array with computed ids and texts | Sketch 2 | | |
| `lv::for_each` flattened by `operator[]` | | | Yes: the sketch used `repeat(parent, f)` |
| Spin box with its state as template argument | Sketch 5 | | |
| Option list joined at compile time | Sketch 3 | | |
| Enum state bound to `.selected` | Refused today, Sketch 3 | | The library change |
| `options_static` for constant lists | | | Yes |
| Strings file, keys checked, language switched | Sketch 4 | | |
| A switch without a text copy | | | Yes |
| Constant sub-tree from a header in a page tree | Sketches 2 and 5 nest constant trees | | The queue view itself |
| Compile cost of looks | Sketch 1, instructions | | `large_tree`: not measured |
| The v10 pin | | | All: the calls used exist there |
| The target home and ink pages as wholes | | | Yes: counted, not compiled |

## Not confirmed

- The height of the opened details in a headless test, with looks and with today's styles.
- The compile time of a page that holds the queue view's tree.
- The cost of a `look` member in `kept<W>` to `tests/large_tree.cpp`.
- The heap of a look-based page on a Cortex-M4.
- The details of Slint's translation flow.

## Decisions for the owner

1. Where is `.look` resolved?
   Options: a `const char*` node member read in `apply_node`; `lv::look<"path">` as a style value; a template argument of the factory.
   Recommendation: the node member, after `large_tree` has measured it; `lv::look<"path">` first, as the slice that needs no node change.
   Cost: one member in every node's `kept<W>`.
2. How does a tree find the tokens?
   Options: a project registration as `lv::cfg` (`lv::design<lv::override>`); a template argument of `lv::mount`.
   Recommendation: the registration; the display then gets the same design for modes.
   Cost: one specialization per project, before the first use.
3. How does an element take a variant, as tile and tile.print?
   Options: several looks in one string, `"tile tile.print"`; `look<a> | look<b>`; a path that takes its parents' entries.
   Recommendation: several in one string, later ones win.
   Cost: a parse at mount, and one more rule to learn.
4. How does text style reach a label?
   Options: LVGL's inheritance from the parent's look; a cascade by name (`tile.label` for a label in a tile).
   Recommendation: inheritance, with a text role look for the exceptions.
   Cost: a label's look depends on its parent's look, as in LVGL themes.
5. Do size and layout go into looks?
   Options: fixed sizes, padding and flex in looks, positions and percentages in the tree; everything in looks with a percent extension; none.
   Recommendation: the first.
   Cost: computed sizes become numbers in the token file.
6. How does a token file name an enumerator, such as `"center"` or `"flex"`?
   Options: `"$type": "string"`, which lvgl26 reads and DTCG lacks; keep enumerators in the tree.
   Recommendation: `"string"`, checked against the property's enum at compile time.
   Cost: a token file that other DTCG tools may warn about.
7. Does a look that sets `bg_color` without `bg_opa` get an opaque background?
   Options: yes, by rule; no, the look says `bg_opa`.
   Recommendation: no: the look says it once, which removes the 25 `tok.solid` from the code anyway.
   Cost: one entry per filled look.
8. What is the repetition API?
   Options: `lv::for_each<E>(f)`, `lv::for_each_field<T>(f)` and `lv::for_each<range>(f)`, flattened by `operator[]`; a structural node.
   Recommendation: the flattened form: the result is the same tree as written by hand.
   Cost: one overload of `operator[]` and three names.
9. Where do captions of a row type live?
   Options: annotations on the members; a `constexpr` table of `{caption, member}`.
   Recommendation: annotations whose caption is a strings key.
   Cost: the row type names UI keys.
10. How are option lists written?
    Options: `lv::options{...}` joined at compile time; `options_of<E>` with `.selected` bound to an enum state; both.
    Recommendation: both, with `options_static` chosen for constant lists.
    Cost: an enum state binding to an index, a library change.
11. Where do texts come from?
    Options: our strings file, checked at compile time, switched by a state; LVGL's `lv_translation`; texts in code.
    Recommendation: our strings file, with a static-text binding so that a switch copies nothing.
    Cost: a reader, `lv::tr`, and the binding that decision 15 also needs.
12. How does a sub-tree of another file join a page?
    Options: a constant tree in a header, in the page's tree; a `lv::part` with declared storage; today's mount and `release()`.
    Recommendation: the constant tree now; `lv::part` when a file must keep its tree out of the page's compile.
    Cost: the page's file compiles the sub-tree.
13. In which order?
    Recommendation: sub-trees (no library change), `lv::look<"path">` with the design registration, `lv::for_each`, options, then `.look`, texts and captions.
    Cost: the port changes in several steps.

## What the owner decided

Decided 2026-11-18, in an interview over the list above.

| # | Decision |
|---|---|
| 1 | Named looks stay, although they mostly move words: the code says what an element is, and the token file says how it looks. First as a style value, `.style = lv::look<"queue.card">`. A `.look` member of every node comes after its cost to `tests/large_tree.cpp` is measured. |
| 2 | A project names its token file once, in the style of `lv::cfg`. No token argument is passed around. |
| 3 | A variant is several looks in one string, and a later one wins: `"tile tile.print"`. |
| 4 | Text style reaches a label by LVGL's inheritance from the look of its parent. |
| 5 | A look may carry fixed sizes, padding and flex settings. Positions and percentages stay in the tree. |
| 6 | An enumerator in the token file is a string, which lvgl26 reads and checks. |
| 7 | A look that sets `bg_color` does not set `bg_opa` by itself. |
| 8 | Repetition is `lv::for_each<E>(part)` and `lv::for_each_field<T>(part)`. |
| 9 | A caption is an annotation on the member, and its value is a key of the strings file. |
| 10 | An option list is joined by the compiler: `.options = {"100%", "75%", "50%"}`. `lv::options_of<E>` makes one from an enum, and `.selected` binds to a state of that enum. |
| 11 | Texts come from our own strings file, one file for each language, checked by the compiler. |
| 12 | A sub-tree is a tree constant in a header for now. `lv::part` comes later. |
| 13 | The levers that remove words come forward: repetition, option lists and `lv::look` as a style value are built beside the storage work. Then `.look`, strings and captions. |

## Built: lv::look (2026-11-18)

Lever 1, the first slice: `lv::look<"path">` as a style value, with the registration `lv::design<lv::override>`. `docs/looks.md` is the page, `include/lv/look.hpp` the header, `tests/look.cpp` and `tests/fail/look_*.cpp` the tests. `.look` as a node member is not built, as decision 1 says: the first measure of its cost to `tests/large_tree.cpp` is still to be taken.

What the build showed, beyond the sketches:

| Finding | Mark |
|---|---|
| `alt` is an LVGL state (`lv_state_t` in `docs/reference/lvgl-v9.6.0.json`), so a variant group named `alt` is read as a layer. The parts and states are the names a nested group may not use for a look of its own | `measured` |
| A non-template consteval function that calls `std::meta::members_of(^^::, ...)` with constant arguments runs the scan where the header is read, in every file: 0.8 G instructions, more than the whole feature. Taking the scope as a parameter moved it to the one call that needs it | `measured` |
| A property that LVGL types as a number and that names an enum (`layout`) needs the enum by name, `lv_layout_t`; found once for each property, not once for each look (10 looks with two such entries: 76.1 G, then 71.3 G) | `measured` |
| `std::string_view` is no element for `std::define_static_array` (`reflect_constant_array` wants a structural type); a loop over a list of names uses an index and a constant range | `measured` |
| A column of flex items with one track sits at the left of its container, however `flex_cross_place` is set: the track needs `flex_track_place: "center"` | `measured` |
| A shorthand (`pad_all`, `pad_gap`) has no style id, so a constant style cannot hold it and a node would set it as a local style, which beats a layer. A look stores the long forms | `source`, `style.hpp`; `measured` in `look.cpp` |
| `lv::group` set a size of 100 percent as a node property, which beat the look's width. It now leaves a side to the look that sets it, and `lv::row` and `lv::column` leave the flow | `measured` |
| A look adds no heap beyond LVGL's own list of styles: a box with two looks takes the heap of a plain box; a look of three layers took 40 bytes more | `measured` |
| `lv::look` is found in a theme (`.obj = {.style = lv::look<"padded">}`) and its constants are shared like those of a style | `measured` |
| The editor stub of a token file lists its groups, so Clang marks a look that names none (`editor_wrong_look`); it cannot read the path of `lv::look_of`, whose last name comes from an enumerator | `measured` |

### What it cost (instructions of `cc1plus`, `tools/compile_report.py`, GCC 16.2, `host-v96`)

| What | Instructions | Mark |
|---|---|---|
| A file that only includes `<lv/lv.hpp>`, with and without `look.hpp` | 36.69 G and 36.60 to 36.73 G (main, two runs): the noise is 0.15 G | `measured` |
| `tests/large_tree.cpp` | 106.39 G, main 106.61 G; the limit of 6,500,000 operations holds | `measured` |
| `tests/smoke.cpp` | 18.81 G, main 18.90 G | `measured` |
| A look of five plain entries, in a file of 10 | about 0.6 G | `measured` |
| One more layer; a shorthand; two enumerator entries | 0.25 G; 0.08 G; 0.38 G | `measured` |
| One typography entry | 6.6 G, against 3.0 G for the same font written in the tree | `measured` |
| 1, 10, 50 boxes, each with a look of nine entries, a layer and a font | 152.0 G, 211.2 G, 471.7 G | `measured` |
| The same boxes with the style written out | 146.8 G, 173.1 G, 289.9 G | `measured` |
| Heap: a box with two looks, a box with a look of three layers, a plain box | 296, 336, 296 bytes | `measured`, `lv_mem_monitor` |
| Objects of `lv::look<...>` left in a program built with `-O2` | none | `measured`, `nm` |

A look costs about twice what the same style costs written out, and nearly all of the difference is the font: `lv::typeface` draws the font at each call, and a look makes the call once for each look that names a typography token.
A font made once for each size and weight, and shared by the looks, would cut it. `reasoned`

### Why a font cost twice in a look, and the fix (2026-11-18)

Pieces, by stubbing one at a time in a copy of the headers (instructions, `fw_*` files of 1 and 10 boxes with one typography entry):

| Piece | Cost for each look | Mark |
|---|---|---|
| Path lookup, entry-to-property match, merge, with the font stubbed to null | 0.36 G | `measured` |
| `typeface::operator()` called inside the look | 6.5 G; the same call written in the tree 1.5 to 3.0 G | `measured` |
| The same call with `-fconstexpr-cache-depth=40` | 2.7 G for each look, and the tree is cheaper too | `measured` |

Cause: GCC caches the result of a constant call only down to `-fconstexpr-cache-depth` levels of nesting, 8 by default. A call in the tree is shallow and a repeated font is a cache hit. A look reaches the font through the walk, the group, the layer and the entry, 12 to 14 levels, so every look drew its font again. The call was not run twice (counted with `__builtin_constexpr_diag`).
Fix: `drawn_font<D, font_key{file, px}>` in `look.hpp` is a variable template, so the compiler draws each file and size once for the program, whatever the nesting. The walk now passes each group and token as a template argument (a reference to a sub-object of the registered tokens), which makes the file and size constants. `lv::typeface` gets `file_of` and `draw`, and `operator()` is made of them.

| Boxes, each with a look of nine entries, a layer and a font | Before | After | Written out |
|---|---|---|---|
| 1 | 152.0 G | 153.0 G | 146.8 G |
| 10 | 211.2 G | 155.8 G | 173.1 G |
| 50 | 471.7 G | 169.6 G | 289.9 G |
| 10 looks, 10 different font sizes | 608.2 G | 610.8 G | 540.5 G |

A look that asks for a font nobody drew yet still costs about 7 G more than the tree, 13 percent of the drawing; the cause is not found. `reasoned`: an instantiation of the walk for each token.

### Decisions of the owner after the review (2026-11-18), as built

The sources are `docs/research/dtcg.md` (pull request 114) and an interview.

- A group nested in a look is a part or a state, and nothing else: any other name is a compile error with a "did you mean" and the lists of parts and states. A variant is a sibling group (`tile-print` beside `tile`), because Format 6.4.4 calls a group that extends its own parent circular, and a name holds no dot (5.1.1). That removes the edge where a variant named `alt`, `knob` or `focused` was silently a layer.
- `lv::look<"tile tile-print">` stacks looks, a later one wins; `lv::look_of<"tile", Page>` is `tile-print`.
- `$extends` is wanted and belongs to the token reader, in the work that implements 2025.10 fully. Until then the reader marks a group that has it (`lvgl26_extends` member), and a look of it is a compile error.
- The token file holds no custom `$type`. An LVGL value by name and a percentage are in the group's `$extensions` under `io.github.filipsajdak.lvgl26` (Format 5.2.3: reverse domain notation, tools keep it). The reader makes a struct of the strings (`lvgl26_vendor` member); a value that is not a string is a reader error. A string token and a percentage `$value` in a look are compile errors that point there.
- A color written `#rrggbbaa` is a `token<rgba_color>` and sets the opacity beside its property (`bg_color` with `bg_opa`; `image_recolor` with `image_recolor_opa`); an opaque color sets none, and an explicit opacity entry wins. This reverses decision 7 for a color that carries alpha. Colors as objects are left to the reader work.
- `lv::look_of` and the look paths with a dash: the compiler spells `-` as `_`, so a message lists `tile_print`.
- Percentages in the standard: see `docs/research/dtcg.md`; no percentage is refused or documented as final.
