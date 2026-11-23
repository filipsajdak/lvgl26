# Fonts drawn once: design

Status: a design to interview the owner on. Nothing is built. Marks: `measured`, `source`, `reasoned`.

## The problem

The compiler draws a font in the file that asks for it. Two files that ask for the same font draw it twice, and only the linker merges the two copies afterwards (`inline constexpr` variable templates). Measured on one machine (docs/fonts.md): 3.5 s for 14 px, 7.5 s for 48 px of Barlow, and 168 G instructions for `lv/signal_type.hpp` in every file that includes it (docs/research/header-structure.md). `measured`

`drawn_font<D, font_key>` in `include/lv/look.hpp` already draws each file and size once for one translation unit. It cannot cross files. The owner wants once for the program, with one file per font (docs/research: decisions of 2026-11-20, "projects split into files").

## The idea

The address of an object that has external linkage is a constant, whatever its contents. A file that only needs the address of a font (to put it in a style) needs no drawing: it declares the object. One file defines it, and draws it.

```cpp
// design/fonts.hpp: every file includes it. No file, no size, no bytes.
namespace fonts {
extern const lv::drawn_font title;       // declaration only
extern const lv::drawn_font reading;
}

// design/font_title.cpp: the only file that embeds the TTF and pays the drawing.
#include "fonts.hpp"
#include "design.hpp"
constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
const lv::drawn_font fonts::title = lv::draw(lv::typeface({{"Barlow", 500, barlow}}), tok.text.title);

// a screen: names the font, draws nothing
lv::label({.style = {.text_font = fonts::title}, .text = "Wake up"})
```

`lv::drawn_font` is a constant `lv_font_t` with the type of a font (one struct, not a template), so the declaration needs nothing else and a style takes `&fonts::title` as a constant address. `lv::raw(font)` stays for a font no token names. `reasoned`, one point to prove by a spike (below).

What this gives: a file that uses a font costs a declaration (about zero); the file that defines it costs what the font costs today; with eight fonts in eight files the build can draw them in parallel. ROM is unchanged: the linked image already holds one copy. `reasoned`

## Open choices

1. **How a name ties to a typography token.** Today a style takes `type(tok.text.title)`, which keeps "a font comes from a token". With `extern`, the definition file keeps that link (`lv::draw(type, tok.text.title)` rejects a non-token), and the screens name the object.
   - a. Hand-written declarations as above. Simple; a name and a token can drift apart.
   - b. The declaration comes from the token file by reflection: `lv::font_of<"text.title">` is declared for each typography token, and `font_title.cpp` defines it: `lv::define_font<"text.title">(type)`. No name to keep in step; the tokens list the fonts. Costs a scan per file that uses it (lazy, behind a template parameter, as `props_of`), to be measured.
   - Recommendation: b, if the scan stays under the 0.25 percent rule; else a.
2. **A missing definition.** A font declared and not defined is a linker error with a mangled name. Options: a CMake function `lvgl26_fonts(target ...)` that fails at configure time when a token has no defining file (like `check_includes`), or accept the linker's text. Recommendation: the CMake check, off with an option.
3. **The single-file program.** Examples and the quick start are one file. Keep `type(token)` in place for them (it draws in the file), so both forms exist. Or make `lv::drawn_font` also definable inline (`inline constexpr`). Recommendation: keep both; the docs teach the in-place form first and the split form in "An application that grows".
4. **Themes.** A theme holds styles built in constant evaluation; a font in a theme becomes the address of an extern object, a constant initialiser resolved by the linker. `reasoned`; to prove with `host-v96-asan`, whose GCC 16.2 refuses some address comparisons in constant expressions (CLAUDE.md).
5. **Signal.** `lv/signal_type.hpp` becomes a header of declarations plus a compiled target `lvgl26::signal_type` (one .cpp per Signal font) that a project links; the 168 G leaves every consumer file. A CMake option, off by default like `LVGL26_TTF`. The header-only form stays for one-file programs. Question: does the library target belong in the install of lvgl26, and must the source files build with the user's compiler flags (they do: same GCC)?
6. **`lv::scaled_font`** (TinyTTF, size at run time) is out of scope: it draws on the device.

## The spike, before any build

One file, three translation units, GCC 16.2, `-fsyntax-only` is not enough (linking): `fonts.hpp` declares two `extern const lv_font_t`, one .cpp defines them from `lv::make_font`, two others take their address in a constexpr style and in a theme array. Check it compiles and links with `host-v96`, `host-v96-asan` and `host-v10`, and that the consumer files do not instantiate the drawing (count `glyph<...>` symbols and instructions). If a constexpr tree or theme refuses the address, the design changes to a link-time table (a runtime pointer) and that is the point to bring back to the owner.

## How the cost is measured (rules of 2026-11-23: warn 0.25 percent, block 1.0 percent)

- New probe `fonts_split`: a consumer file that uses four fonts through declarations, and the four defining files as one probe each. Expected: consumer near the cost of a file with no font; the definers at today's cost of one font. This adds probes, so it moves no baseline.
- Existing probes that draw fonts in place (`signal_type`, `font`, `font_bytes`) must not rise; a drop above 0.25 percent moves their baseline by the improvement rule.
- Link check, not a threshold: `nm` of the linked example shows one definition of each font, and the size of `.rodata` is the same as the in-place form (ROM unchanged).
- Compile-fail tests: a font declared and never defined (CMake check), a name that is no token (choice 1b), and the non-token font that the existing message already covers.

## Order of work, after the owner answers

1. The spike, with its result in this file.
2. `lv::drawn_font`, `lv::draw`, the declaration form, one example split into files, the compile-fail tests, the probe.
3. The CMake check.
4. Signal as `lvgl26::signal_type`.
5. Docs: `fonts.md`, "An application that grows", the rule with an id, the status page.
