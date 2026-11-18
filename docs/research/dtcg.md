# The Design Tokens Format: stable, draft, tools and what lvgl26 reads

Research for the owner's request, written 2026-11-18, to plan token files that design tools can open.

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-18 unless its row says otherwise.

- `source`: a primary page, a repository file or our own code, read in this session.
  A page read through a fetch tool is marked `source (fetch)`.
- `measured`: run in this session; the row says how.
- `summary`: a search engine's summary of a page that was not read.
- `reasoned`: a conclusion drawn from the sources above, with no test.

The owner's words, in three parts:

- "percentages are likely to be added in to the standard".
- "check the diff between draft and current standard".
- "don't make custom modifications that goes beyond Design Tokens Format so that files with tokens can be opened by design tools".
His second constraint: every mistake, a typo or a wrong type, is a compile error with a clear message.

## Findings in short

- The draft has no percentage type.
  Its section 8.8 is non-normative and word for word the same as in the stable version.
  The section says its authors are "not 100% sure" about it.
- The draft differs from the stable 2025.10 in a handful of places.
  It adds no type, no unit and no property.
  The largest change is JSON Pointer support, from MAY to MUST.
- `$extends`, `$root`, `$deprecated`, `$description`, `$extensions` and `$ref` are already in the stable 2025.10.
- A variant written as a group nested in the group it extends (`tile.print` extends `tile`) is the spec's own invalid circular example.
  A conforming variant is a sibling group.
- No file of lvgl26 conforms to 2025.10 except a few lines in `tests/data/tokens.json`.
  All write colours as `"#rrggbb"` and dimensions as `"8px"`, the forms before 2025.10.
- The official JSON Schema can run in CI today, but it skips values whose `$type` comes from a group.
  A small pre-pass fixes that.

## The sources and their versions

| What | Version and date | Mark |
|---|---|---|
| [Format Module, stable](https://www.designtokens.org/tr/2025.10/format/) | 2025.10, Final Community Group Report, 28 October 2025, "considered stable" | `source` |
| [Format Module, draft](https://www.designtokens.org/tr/drafts/format/) | Draft Community Group Report, 08 September 2026. Its title still says 2025.10. "Do not attempt to implement this version." | `source` |
| [Color Module, stable](https://www.designtokens.org/tr/2025.10/color/) and [draft](https://www.designtokens.org/tr/drafts/color/) | Same dates as the format module | `source` |
| [Resolver Module, stable](https://www.designtokens.org/tr/2025.10/resolver/) and [draft](https://www.designtokens.org/tr/drafts/resolver/) | Same dates. The document's `version` field must be `2025.10` in both. | `source` |
| [JSON Schemas](https://www.designtokens.org/schemas/2025.10/format.json) | `format.json` and `resolver.json`, 2025.10. The format schema's `$schema` property carries the comment "not part of the official DTCG specification". | `source` |
| Frozen W3C copy of the stable format | `CG-FINAL-format-20251028` at w3.org. It differs from the served 2025.10 page in 106 lines, all in examples: the frozen copy still writes `"#blue"`, `"16px"` and `"1px solid"` in the group-extension examples. | `source` |
| Repository `design-tokens/community-group` | Last push 2026-11-01. 98 open issues. The page `/tr/` itself is a 404: the stable version lives at `/tr/2025.10/`. | `source` |

The served 2025.10 page has editorial fixes since 28 October 2025 (the examples above and a copyright line that names the Resolver Module).
The note below treats the served page as the baseline, and the frozen copy as the proof that no normative text moved.

## The diff: stable 2025.10 against the draft of 2026-09-08

Method: both pages converted to text and compared line by line, then each hunk read.
The 254 differing lines of the format module come down to the rows below. `measured`

| # | Where | Stable 2025.10 | Draft | Kind |
|---|---|---|---|---|
| 1 | Status, [format](https://www.designtokens.org/tr/drafts/format/) | Final report 28 October 2025, "considered stable" | Draft report 08 September 2026, "do not implement anything in this document" | status |
| 2 | Format 6.6 and 7.1.2, JSON Pointer | 6.6: tools "MAY also support" JSON Pointer. 7.1.2: tools "MUST support" it. The two contradict. | Both say MUST (PR 442, merged 2026-09-08) | changed |
| 3 | Format 6.6.1 | Heading "Current Reference Syntax (Recommended)" | "(Recommended)" removed | changed |
| 4 | Format 7.1.2, Example 32 | Token-level `"$ref": "#/colors/blue/$value"` plus `"$type": "color"`. It is "equivalent to `{colors.blue}`". | Token-level `"$ref": "#/colors/blue"` points at the whole token. A property reference moves inside `$value`: `{"$ref": ".../components/0"}`. The "Key Differences" table below it still says `#/group/token/$value`. | changed, and the draft contradicts itself |
| 5 | Format 3.3, design tools | Figma, Sketch, UXPin and others | Penpot added (PR 406) | added to a list |
| 6 | Format, about 25 examples | No `$schema` | `"$schema": "https://www.designtokens.org/schemas/2025.10/format.json"` in each | added to examples |
| 7 | Format 6.4.4, Example 17 | `"$value"` and `"$type"` written as `// ...` | `"$value": 1, "$type": "number"` | editorial |
| 8 | Format 9.6, shadow examples | A missing comma between `mixed-reference-shadow` and `inner-shadow` | Comma added, indentation fixed | editorial |
| 9 | Color 4.1, range table | HSL hue "[0 - 360]" | "[0 - 360)" (360 itself is invalid) | corrected |
| 10 | Resolver 4 | Sections 4.1 to 4.4, `$defs` is 4.4 | New 4.4 `$schema`: "does not affect resolution behavior". `$defs` becomes 4.5. | added |
| 11 | Color and resolver, references | Old URLs and dates | New URLs, CSS Color 4 dated 6 August 2026 | editorial |
| 12 | Everything else | | Types, units, `$extends`, `$root`, `$deprecated`, `$extensions`, composite types, resolver syntax and 8.8 "Additional types" are unchanged | none |

Since the 2022 to 2024 editor's drafts the stable version also changed what a file looks like.
These are in both pages now, so the old forms are legacy.

| Change | Where it landed | Mark |
|---|---|---|
| Dimension and duration values became objects `{"value": 16, "unit": "px"}`; unit-less and string forms are gone | [PR 244](https://github.com/design-tokens/community-group/pull/244), merged 2024-10-24 | `source` |
| JSON value types were removed, and a token with no `$type` (own or inherited) is invalid | [PR 201](https://github.com/design-tokens/community-group/pull/201), merged 2023-02-07 | `source` |
| Colour became an object with `colorSpace`, `components`, optional `alpha` and `hex` | Color module 4.1 | `source` |
| `$root`, `$extends`, `$deprecated` and group `$type` rules | [PR 298](https://github.com/design-tokens/community-group/pull/298), merged 2025-10-15 | `source` |

## Answer 1: percentages

### What the draft says

[Section 8.8, "Additional types"](https://www.designtokens.org/tr/drafts/format/#additional-types) is marked "This section is non-normative".
It lists three types "still to be documented": font style, percentage or ratio, and file.
The entry reads: "Percentage/ratio: e.g. for opacity values, relative dimensions, aspect ratios, etc."
The note under it reads: "Not 100% sure about this since these are really 'just' numbers."
It names an alternative: widen the `number` syntax "so for example '1:2', '50%' and 0.5 are all equivalent". `source`

The stable page has the same section, word for word.
There is no type name, no value shape and no unit list in either.
The draft does not move the percentage forward. `source`

### How stable it is

- The dimension type accepts `px` and `rem` only, in the text (8.2.1) and in the schema.
  A `%` unit fails the official schema. `measured` with the bundled `format.json`.
- No open issue or pull request asks for a percentage type. `source` (GitHub search of issues and pull requests for percent, %, ratio and opacity).
- [Issue 245](https://github.com/design-tokens/community-group/issues/245), open since 2024-08, argues over which units to allow.
  Proposals there include a reserved list with `percent`, vendor-prefixed units, and any unit at all.
  No decision. `source`
- [Issue 218](https://github.com/design-tokens/community-group/issues/218) holds a maintainer's wish for "more relative units (E.g.
  `%`, `vw`, `vh`, `ch`...) in the future".
  PR 244 rejected a move to number-only dimensions. `source`
- A percentage needs a reference box.
  LVGL's percent is a share of the parent's content area for sizes and of the object itself for `translate_*`.
  A dimension in `px` has no such reference, which is one reason the editors hesitate. `reasoned`

So there are two candidate shapes, and the editors have not chosen.
A unit `{"value": 50, "unit": "%"}` extends dimension.
A string `"50%"` extends number.
Our reader accepts both today for a dimension (`include/lv/tokens.hpp` 289 to 290, `tests/tokens.cpp` 85 to 86). `source`

### Which tools read a percentage

| Tool | Reads `%` as a dimension unit? | Mark |
|---|---|---|
| Figma import | No. "The token's unit value must be set to px. Other values are not supported." | `source (fetch)`, [help article](https://help.figma.com/hc/en-us/articles/15343816063383-Modes-for-variables) |
| Terrazzo 2.10.0 | Rejected by default: the rule `core/valid-dimension` has `allowedUnits`, and a user may widen it. | `source`, `valid-dimension.ts` in the repository |
| Style Dictionary 5.8.0 | No claim found. Its DTCG notes list colour, dimension and duration only. | `source` |
| Penpot (develop branch) | Opacity accepts "45%" and line height accepts a percentage in its user guide, as its own token types, not as DTCG dimensions | `source (fetch)`, [user guide](https://help.penpot.app/user-guide/design-systems/design-tokens/) |
| Tokens Studio | Its own types use `%` for opacity and line height. `sd-transforms` turns them into a number and a unit-less value. | `source`, README of `sd-transforms` |

No tool reads a percentage as a DTCG dimension today. `reasoned` from the rows above.

### Recommendation

1. **Do not read a percentage from a token file by default.** The compile error names the cause and the choices.
2. **Opacity is not a percentage problem.** `number` from 0 to 1 is standard, and every tool reads it.
   The looks already use it.
3. **A size in percent stays in the tree**, as `docs/research/less-boilerplate.md` decided, until the standard has a form.
4. **A named setting admits the form we guess**: `lv::config::tokens_percent_unit`, off by default.
   It accepts `{"value": 50, "unit": "%"}` and nothing else.
   The string `"50%"` is never accepted, because a number token must be a JSON number.
5. **Which properties.** LVGL's documentation allows a percentage for `width`, `height`, `min_*`, `max_*`, `x`, `y`, `transform_width`, `transform_height`, `translate_x` and `translate_y`.
   It allows none for a padding, radius or border property. `source`, `docs/src/common-widget-features/styles/style-properties.mdx` in `build/_src/lvgl-v9.6.0`.
   A percentage on any other property is a compile error even with the setting on.
6. **A version switch** is the setting itself.
   When the standard fixes a form, the setting reads that form, and the old guess becomes an error with a message that names the replacement.

The messages:

```text
lv: design tokens: 'space.half' is 50% of its parent. Design Tokens Format 2025.10 has no percentage
(its section 8.8 lists one as not yet written). Keep the size in the tree (50 * lv::pct), or set
lv::config::tokens_percent_unit to read {"value": 50, "unit": "%"}, which design tools may reject.

lv: design tokens: 'pad_all' takes pixels. LVGL accepts a percentage only for width, height, min and
max sizes, x, y, translate_* and transform_width/height.
```

## Tool support

What each tool reads of the 2025.10 forms, from its own documentation or source.

| Tool | Version and date | Reads 2025.10 forms | Not read or not found | Mark |
|---|---|---|---|---|
| Style Dictionary | 5.8.0, 2026-11-03 | Colour, border and shadow with the colour module. Dimension objects. Duration objects since 5.7.0 (2026-10-16). | Gradient in progress. Resolver module not yet. Its page says "the latest format 2025.10 does not have full support yet" ([issue 1590](https://github.com/style-dictionary/style-dictionary/issues/1590), open). No `$extends` or `$ref` support found in its issues. | `source` |
| Terrazzo | 2.10.0, 2026-10-21 | "Full support for the DTCG v2025.10 spec, including resolvers" (changelog). Object dimensions are required. Group `$extends` works ([issue 679](https://github.com/terrazzoapp/terrazzo/issues/679), fixed in PR 683). | Adds its own `link`, `boolean` and `string` types, "not a supported token type outside of Terrazzo". | `source` |
| Penpot | `develop` at `14abaf93f4`, release 2.19.2 on 2026-11-16 | The code comment says it reads "the 2022 drafts and the Format Module 2025.10". Export has a `:dtcg` profile for 2025.10 values. Import reports errors for bad `$extends`, `$root` and `$type`. | The default export uses Tokens Studio style types (`borderRadius`, `spacing`, `sizing`, `opacity`, `boolean`, `string`). The code says a `:dtcg` file "is not meant to import back unchanged". The user guide names no DTCG version and does not mention `$extensions` or `$extends`. | `source` (code), `source (fetch)` (guide) |
| Figma | Help article read 2026-11-18 | Import of DTCG JSON: colour in sRGB or HSL; dimension in `px`; duration in `s`; font family as one name; number as number or, with `com.figma.type`, boolean. One mode per file. | A `string` type "not an official DTCG type, but accepted". Arrays of font families, `rem`, `%`, composite types and `$extends` are not read or not mentioned. Native export reached users in December 2025 per a forum thread. | `source (fetch)`; forum `source (fetch)` |
| Tokens Studio | Docs read 2026-11-18; `sd-transforms` 2.0.3, 2025-12-10 | Moves "towards the W3C Specifications". Prefixes `$value`, `$type`, `$description`. | Names no DTCG version. Says it will support "additional token types ... in future releases". Its own types differ from DTCG, and `sd-transforms` converts them. | `source (fetch)`, `source` |
| Supernova | The exporter repository was last pushed 2022-10-21 | A DTCG exporter from the 2022 drafts | No page found that claims 2025.10 | `source` (repository), `summary` (search) |
| Specify | Blog post, undated | Intends "to fully adapt" to the DTCG spec; has its own format | No page found that names a DTCG version | `summary` |
| Others on the [DTCG adopters list](https://www.designtokens.org/) | Page read 2026-11-18 | Adobe, Framer, Sketch, zeroheight, Knapsack, TokensBrucke, Zeplin, Design Tokens Language Server, Design Token Validator and more | Not read in detail. The language server and validator are the two to try for our files. | `source` (list only) |

Two facts to carry forward.
Every tool above that reads colour and dimension reads the object forms, so the string forms we write today are the ones at risk.
A tool that reads `$extends` is the exception, not the rule: Terrazzo and Penpot do, and no document says the others do.

## Answer 2: the features we do not use yet

### Variants in plain DTCG: `$extends` and its trap

Can a variant such as `tile.print` be a group that extends `tile` with no custom keys?
Yes, with one restriction that decides the naming.

- [Format 6.4](https://www.designtokens.org/tr/2025.10/format/) defines `"$extends": "{group}"` as a deep merge: local tokens replace inherited ones at the same path, other paths are added.
  A replaced token is replaced whole.
- Format 6.4.4 lists a nested group that extends its own parent as invalid ("Invalid: circular reference", Example 16: `button.secondary` extends `{button}`).
  `tile.print` extending `{tile}` is that case. `source`
- So a variant is a **sibling** group: `tile-print` next to `tile`, or `tile_variants.print`.
  The name cannot contain a dot: `{`, `}` and `.` are forbidden in names (5.1.1).
- `$extends` accepts the same references as aliases, curly braces or a JSON Pointer (6.4.5).
  The target must be a group, never a token (6.4).

```json
{
  "tile": {"radius": {"$type": "dimension", "$value": {"value": 16, "unit": "px"}},
           "pressed": {"bg_opa": {"$type": "number", "$value": 0.8}}},
  "tile-print": {"$extends": "{tile}", "bg_color": {"$type": "color", "$value": "{color.blue}"}}
}
```

A look would be `lv::look<"tile-print">`, whole by itself, where today it is `lv::look<"tile tile.print">`.
`pressed` is inherited, because groups merge by path. `reasoned` from 6.4.3.

The second plain form needs no `$extends`.
Keep `tile` and `tile.print` as they are, as two groups of overrides, and combine them in our code.
Every tool reads that file, because it uses no inheritance.
Our rule that `lv::look<"tile tile.print">` stacks them is our convention, which no tool needs to know.

Which is safer for design tools: the second.
Only Terrazzo and Penpot are documented to read `$extends`.
Format section 6 also says tools "SHOULD NOT" infer the type or purpose of a token from its group.
Our reader reads state and part names (`pressed`, `knob`) from groups.
That does not make a file invalid.
It means no other tool shows those meanings.

The risk in type inheritance: 6.7.3 says that with `$extends` the type follows "JSON Schema constraint resolution".
Example 22 leaves a group with two types.
Our reader would say: an extended group's `$type` must equal its target's, else error. `reasoned`

### The other features

| Feature | What the spec says | What it gives lvgl26 | State today |
|---|---|---|---|
| Group `$type` | Default for tokens below; a token's own type wins (5.2.2, 6.7.3) | One word less per entry | Read (`tokens.hpp` 189, 393) |
| `$root` | A token named `$root` is a group's base value; alias `{color.accent.$root}` (6.2) | A base colour with variants under one name | Dropped silently (line 152) |
| `$description` | Plain string; IDEs "MAY display the description as a tooltip" (5.2.1) | Hover text in the editor stubs, from `tools/editor/generate.py` | Ignored, which is allowed |
| `$deprecated` | `true`, a string, or `false`; on tokens and groups (5.2.4, 6.3.1) | A marked token in the stubs, and a check in CI. GCC has no compile-time warning channel that a token read can use. | Ignored |
| `$extensions` | Vendor key, "reverse domain name notation is recommended"; tools "MUST preserve" data they do not understand (5.2.3). Data should be "optional meta-data that is not crucial to understanding that token's value". | A home for lvgl26 hints such as the list of components. It is inside the format, if it holds only hints. | Skipped (`$` keys). `feat/look` writes `$extensions.lvgl26.components`, a key that is not a reverse domain. |
| Aliases | `{group.token}`; a token's type is the resolved type (5.2.2) | Done | Read (`follow_alias`) |
| JSON Pointer `$ref` | Stable: 6.6 says MAY, 7.1.2 says MUST. Draft: MUST. Token-level `$ref` and `$value`-level `$ref` (Example 32). | Property-level aliases, such as the red part of a colour | A `$ref` token is read as an empty group (line 156) |
| Composite types | `shadow`, `border`, `typography`, `transition`, `gradient`, `strokeStyle` (9) | Done in part | Shadow keeps the first of a list and ignores `inset`; `fontWeight` takes two names |
| `cubicBezier`, `duration`, `transition` | Four numbers; `ms` or `s`; a duration, a delay and a timing function (8.5, 8.6, 9.5) | Animation in a token file | Read |
| `fontFamily`, `fontWeight`, `number` | One name or a list; 1 to 1000 or 18 names; a JSON number (8.3, 8.4, 8.7) | Done in part | `fontWeight` takes `normal` and `bold` only |
| Resolver module | Sets, modifiers with `contexts` and a `default`, `resolutionOrder`; an input of strings, such as `{"theme": "dark"}`. | Light, dark and sporty as contexts of one modifier | Not read. One file per mode today. |
| Color module | `colorSpace`, `components`, `alpha`, `hex` fallback; 14 colour spaces; `none` keyword | Any design tool's colour | `srgb` only, in effect (see answer 3) |
| `$schema` | Not part of the spec; the schema tolerates it | Editor validation of our token files | Root `$` keys skipped, so it is allowed |
| Strings, translations | No string type in either version. [Issue 234](https://github.com/design-tokens/community-group/issues/234) (open since 2024) asks for `string` and `boolean`. [Issue 396](https://github.com/design-tokens/community-group/issues/396) asks for translated `$name` and `$description`. | Nothing: the owner decided on a separate strings file | Consistent |

### The resolver, and what `#embed` allows

A resolver document is a JSON file.
Its sets and modifiers point at token files with `{"$ref": "themes/dark.json"}`, or hold the tokens inline.
For a mode `theme` with contexts `light`, `dark` and `sporty`, the input `{"theme": "dark"}` selects the files of that context.
The schema for the resolver accepts `contexts`, not `context`, although two of the spec's own examples write `context`. `measured` with `resolver.json`.

The compile-time limit: `#embed` takes a path written in the source, and a constant-evaluation reader cannot open a file.
So a resolver document cannot make the compiler fetch `themes/dark.json`.
Three ways remain. `reasoned`

1. **A bundled document.** The resolver's section 7 describes bundling: replace each `$ref` with the file's contents.
   A tool writes `x.bundle.resolver.json`, and we embed that one file.
2. **A file map.** The project embeds each file and names it: `lv::resolver<resolver_json, files>`, where `files` pairs a path with its embedded array.
   A path the map lacks is a compile error.
3. **Inline sources only.** The resolver names no file.
   Simple, but design tools usually write files.

The run-time part follows `docs/research/shown-theme-format.md`, decisions 11 to 13.
Each permutation is one constant token struct, the tree is written once, and a state selects the struct.
The compile-time check is the one already decided: every context yields the same token paths and types.
Permutations multiply (the resolver's 4.1.5.4: contexts per modifier multiplied), so only the permutations that the program names are built.

### The colour module for an 8-bit sRGB target

- LVGL's `lv_color_t` is 8-bit sRGB. `srgb` components times 255, rounded, is exact for every value a tool exports. `source` (`tokens.hpp` 277 to 283).
- `hsl`, `hwb` and `srgb-linear` convert with simple constant math.
  `oklab`, `oklch`, `lab`, `lch` and the wide gamuts need matrices; they can be done in constant evaluation. `reasoned`
- Gamut: Color module section 5 is non-normative and leaves the mapping to the tool.
  Our rule: a converted component outside 0 to 1 by more than one 255th is an error.
  The exception is a colour with a `hex`, which the spec calls a "fallback value".
  Then the hex is used and nothing else.
- Today `hex` wins over `components` (line 276).
  That reverses the spec: components are the value, hex is the fallback.
- `none` in `components` means a missing component.
  Reading it as 0 is the CSS rule for a colour that is not interpolated.
- Alpha: LVGL colours carry none.
  Colour tokens with `alpha` below 1 stay an error, as `#rrggbbaa` is now.
  The composite tokens (shadow, gradient stop, border) take the alpha as their opacity.

## Answer 3: what in our files and reader is outside the standard

Measured with the official `format.json`, after a pre-pass that gives each token its group's `$type`.
Without the pre-pass, three of six files tried (`include/lv/signal/day.json`, `examples/lists/tokens.json`, `examples/theme/tokens.json`) pass although their colours are hex strings.
The schema checks a value only when the token names its own type ([issue 439](https://github.com/design-tokens/community-group/issues/439), open). `measured`

### In the token files

Counts are tokens of that form.
Composite sub-values count the strings inside typography, border and shadow values.

| File | Tokens | `"#rrggbb"` colours | `"8px"` dimensions | String sub-values of composites | First line |
|---|---|---|---|---|---|
| `include/lv/signal/day.json`, `dark.json` | 29 each | 15 | 14 | 0 | 6 |
| `include/lv/signal/type.json` | 10 | 0 | 0 | 16 | 17 |
| `examples/lists/tokens.json` | 11 | 6 | 4 | 0 | 5 |
| `examples/theme/tokens.json` | 14 | 5 | 7 | 0 | 5 |
| `examples/fonts/tokens.json` | 5 | 0 | 0 | 6 | 10 |
| `examples/lvgl/get_started/tokens.json` | 15 | 9 | 2 | 2 | 15 |
| `examples/lvgl/widgets/tokens.json` | 137 | 53 | 68 | 41 | 15 |
| `examples/ports/qt/treadmill/tokens.json` | 66 | 41 | 12 | 16 | 14 |
| `examples/ports/slint/printer/tokens.json` | 93 | 53 | 20 | 16, plus 3 duration strings | 14 |
| `tests/data/tokens.json` | 46 | 5 (1 in object form) | 5 (1 in object form) | 10, plus 1 duration string | 7 |
| `feat/look`: `examples/looks/tokens.json` | 54 | 12 | 8 | 6, plus 10 tokens of `"$type": "string"` | 10 |

The conforming form of each:

| Written now | 2025.10 form |
|---|---|
| `"$value": "#3366ff"` | `"$value": {"colorSpace": "srgb", "components": [0.2, 0.4, 1], "hex": "#3366ff"}` |
| `"$value": "8px"` | `"$value": {"value": 8, "unit": "px"}` |
| `"delay": "0ms"` | `"delay": {"value": 0, "unit": "ms"}` |
| `"$type": "string", "$value": "flex"` | None. See decision 2. |
| `"$value": "50%"` or a `%` unit | None. See answer 1. |

`design/signal/tokens.design.json` (306 lines) is not DTCG.
It holds `{"name", "value", "usage"}` arrays and a `themes` list, and `tools/signal_tokens.py` turns it into `day.json`, `dark.json` and `type.json`.
The generator leaves out `radius-face` and every non-pixel length.
That file is our own authoring format, which is allowed if it is not a file for design tools.
See decision 9. `source`

### In the reader, `include/lv/tokens.hpp`

| Line | What it does | The standard says | Replacement |
|---|---|---|---|
| 52 | Reads `"$type": "string"` | No such type (8). The schema rejects it. | None; decision 2 |
| 53, 316 | Reads `strokeStyle` as text, and an object as `"custom"` | An object with `dashArray` and `lineCap` (9.3.2) | Read the object |
| 156 to 157 | A token whose `$type` is unknown or missing is left out without a word | "The token MUST be considered invalid" (5.2.2, 8) | A compile error that lists the known types |
| 152 | Every `$` key of a group is skipped, so `$root` tokens and `$extends` vanish | `$root` is a token; `$extends` is inheritance | Read both; until then an error |
| 156 | An object without `$value` is a group, so a token with `$ref` is an empty group | A `$ref` token is a token | Read `$ref`; until then an error |
| 261 | A bare number is a dimension or duration | The value MUST be an object; "unit is still required even if value is 0" (8.2.1) | Error; legacy behind a setting |
| 259 to 270 | String values `"8px"`, `"200ms"` | Objects only | Error; legacy behind a setting |
| 288, 296 | A missing unit means pixels or milliseconds | The unit is required | Error |
| 290 | Unit `%` | `px` and `rem` only | Answer 1 |
| 276 | `hex` wins over `components` | `hex` is a fallback | Components first |
| 275 to 283 | `colorSpace` is ignored, so an HSL colour is read as sRGB; `alpha` is ignored; `none` is not handled | Components are in the named space | Convert, or error |
| 306 to 311 | `fontWeight` takes numbers, `normal`, `bold` | 1 to 1000 and 18 names; others "MUST be rejected" (8.4) | Table of names, range check |
| 336 | A shadow list keeps its first shadow; `inset` is ignored | A list is a layered shadow; `inset` is a property (9.6) | Keep the list, or an error for a second entry |
| 36 to 61, `look.hpp` | Group names carry no meaning to DTCG, but our looks read `pressed` and `knob` from them | Tools "SHOULD NOT" infer purpose from groups (6) | Allowed; document it as our convention |

Where the standard has no replacement, the table says "None", and the decision list gives the choices.

## Answer 4: a plan

Each slice is one pull request, with a compile-fail test per new error.
Settings are fields of `lv::config` (`include/lv/config.hpp`), because the project allows no macros.

| # | Slice | What it enables | Compile-time checks it adds | Stable or draft risk, and its setting |
|---|---|---|---|---|
| 1 | `tools/check_tokens.py`: run `format.json` and `resolver.json` over every token file, after pushing group types down. Add `"$schema"` to the files. List the files of answer 3 as known exceptions. | A CI gate that says "design tools may reject this" with the tool's own schema | None in C++. The gate fails on a new exception. | Schema is 2025.10 only. Needs `jsonschema` in the CI image. |
| 2 | `tools/dtcg_fix.py`: rewrite strings to objects. `signal_tokens.py` writes objects. All 11 files convert. | Files that Figma, Penpot and Terrazzo can open | Reader accepts both forms. | `tokens_legacy_values`, on while files convert, then off. Off turns `"8px"` into the error below. |
| 3 | A strict reader: unknown or missing type, `$ref` and `$root` and `$extends` with a message, full `fontWeight`, units required, `hex` as fallback, `colorSpace` and `none`, shadow list and `inset` | Every typo and every wrong form is an error at the line of the token | Message for each row of the reader table | Stable only |
| 4 | Colour spaces: `srgb`, `hsl`, `hwb`, `srgb-linear`; later the wide gamuts | Any design tool's export | Out-of-gamut error with the colour and the property | Stable only |
| 5 | `$root` and `$extends`, with cycle, token-target and missing-target errors and the type rule | Variants as sibling groups, whole in one name | Four compile-fail tests: cycle, a token as target, a missing target, a group that extends its own ancestor | Stable. Tools that skip `$extends` show a group without its inherited tokens. |
| 6 | `$description` and `$deprecated` into the editor stubs and a CI report | Hover text; a deprecated token flagged before the build | CI, not the compiler | Stable |
| 7 | The resolver reader: bundled document or file map; equal paths and types in every context | Modes from design tools' own resolver output | Context missing, `context` for `contexts`, a modifier with one context, an input outside the contexts | Stable. Tools write resolver files only in part: Terrazzo does, Style Dictionary does not yet. |
| 8 | `%` for size properties | Percent sizes in a token file | The messages of answer 1 | `tokens_percent_unit`, off. Reads the form the draft may take. |
| 9 | JSON Pointer `$ref` | Property-level aliases | A pointer to a missing path; a pointer into a modifier | Draft risk: stable and draft differ on what a token-level `$ref` points at. Read both: a pointer that ends at a token object is the draft form, one that ends at a value is the stable form. Off by default. |

The legacy error of slice 2:

```text
lv: design tokens: 'space.gap' is written "8px". Design Tokens Format 2025.10 writes a dimension as
{"value": 8, "unit": "px"}. Run tools/dtcg_fix.py on the token file, or set
lv::config::tokens_legacy_values while the files convert.
```

What stays behind a setting: slice 2's legacy forms, slice 8's percent and slice 9's pointer.
We do not follow these issue-tracker proposals until they are in the stable text:

- `em` ([issue 390](https://github.com/design-tokens/community-group/issues/390))
- Plain numbers for durations ([issue 342](https://github.com/design-tokens/community-group/issues/342))
- A `lineHeight` type ([issue 102](https://github.com/design-tokens/community-group/issues/102))
- A motion beyond `transition` ([issue 429](https://github.com/design-tokens/community-group/issues/429)).
The draft should be read again each month with the diff method above.

## Corrections to earlier notes

- `docs/research/less-boilerplate.md` called `{"$type": "dimension", "$value": "24px"}` "the DTCG form".
  The 2025.10 form is `{"value": 24, "unit": "px"}`.
  The note is corrected in place.

## Decisions for the owner

1. **How does a token file carry a percentage?**
   Options: no percentage in files (sizes stay in the tree); `{"value": 50, "unit": "%"}` behind `tokens_percent_unit`.
   More options: a vendor key in `$extensions` beside a pixel value; the string `"50%"`.
   Recommendation: the first, with the second behind the setting for the day the standard picks a form.
   Cost: percent sizes stay written in the tree, and the setting may be wrong when the standard decides.
2. **How does a token file name an LVGL enumerator, such as `"center"` or `"flex"`?**
   Options: `"$type": "string"` behind `tokens_string_type`; enumerators in the tree or in a second file that is not DTCG; a `$extensions` hint on a number token.
   Recommendation: the second, because the standard has no string type and strict validators reject it.
   Cost: a look cannot say `flex_flow` in the token file, so layout stays in the tree.
   Figma, Terrazzo and Penpot do accept a `string` type, so the first works in practice.
3. **Do all token files move to the 2025.10 object forms now (slices 1 to 2)?**
   Options: now; only when a design tool is in use.
   Recommendation: now, with a tool that rewrites the files.
   Cost: object forms are longer.
The widgets example grows by about 120 values.
4. **How long does the legacy string form stay readable?**
   Options: never; behind a setting that is off after the conversion; for good.
   Recommendation: behind a setting, removed one release after the conversion.
   Cost: two forms in the reader for that time.
5. **How are look variants written?**
   Options: stacked groups, `lv::look<"tile tile.print">`, which uses no `$extends`; sibling groups with `$extends`, `lv::look<"tile-print">`; both.
   Recommendation: stacked groups now; `$extends` as a later addition when a design tool that reads it is in use.
   Cost: no inheritance in the file, so design tools show `tile.print` as a partial group.
6. **What is the key under `$extensions`?**
   Options: `lvgl26`; a reverse domain such as `io.github.<owner>.lvgl26`.
   Recommendation: a reverse domain, because the spec recommends it.
   Cost: a longer key in files.
Data under it stays hints only.
7. **What does a colour with alpha mean?**
   Options: an error for a plain colour, with alpha read for composite tokens (shadow, gradient, border).
   More options: alpha always read into a matching `_opa` property; always an error.
   Recommendation: the first.
   Cost: a design tool's semi-transparent fill needs its own opacity token.
8. **How do modes reach the compiler?**
   Options: one file per mode (decision 13 of `shown-theme-format.md`); a bundled resolver document; a file map next to a resolver.
   Recommendation: one file per mode now, the bundled resolver in slice 7.
   Cost: a bundling step in the build.
9. **What is the source of Signal's tokens?**
   Options: keep `design/signal/tokens.design.json` and the generator; make a DTCG file with a resolver the source; both, with the generator reading DTCG.
   Recommendation: the second, so a design tool can open the source itself.
   Cost: the `usage` text moves to `$description`, and `radius-face` (50%) needs the answer of decision 1.
10. **Does CI run the official schemas?**
    Options: yes, with `jsonschema` in `.github/ci-image/Dockerfile`; no.
    Recommendation: yes.
It is the only check that matches what other tools read.
    Cost: one Python package in the image.
11. **What does the reader do with a type it does not know?**
    Options: an error that lists the known types; skip the token silently (today); skip only types in a list in `$extensions`.
    Recommendation: the error, because a typo in `$type` must not pass.
    Cost: a token file with a type from a tool's export must drop it first.
12. **Which text do we follow?**
    Options: the stable 2025.10 only; the draft where it differs.
    Recommendation: the stable text, with the draft's differences behind settings, and a monthly re-read.
    Cost: when the draft changes the pointer form, we read both.
