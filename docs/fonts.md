# Fonts

The compiler reads a TrueType file and makes the font. The device gets glyph pictures in read-only memory: no heap, no code that draws outlines, and no converter to run.

![A screen with the title "Wake up" in Barlow, the time 06:45 in large Barlow Condensed digits, and a line of small text](media/fonts.png)

## Embed the files

<!-- snippet: examples/fonts/main.cpp files -->
```cpp
constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char condensed_ttf[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
```

The file must have TrueType outlines: a `.ttf`. Its licence says whether its glyphs may go into a device; Barlow's does (SIL Open Font License).

## Say which file each family is

A font is part of the design, so its family, weight and size come from a typography token. A typeface says which file a family and weight is.

<!-- snippet: examples/fonts/main.cpp typeface -->
```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto type = lv::typeface({
    {"Barlow", 500, barlow_ttf},
    {"Barlow Condensed", 600, condensed_ttf, "0123456789:"},  // only what a clock shows
});

constexpr auto label = type(tok.text.label);
constexpr auto title = type(tok.text.title);
constexpr auto numeral = type(tok.text.numeral);
```

- `type(token)` makes the font the token asks for. Keep it in a `constexpr` variable: the compiler draws it once.
- A file holds the characters from the space to the tilde. Name others, in UTF-8, as the fourth value. The clock digits above need eleven characters, not ninety-five.
- The same file, size and characters are one font in memory, however often you ask.

The tokens of this example are [examples/fonts/tokens.json](../examples/fonts/tokens.json).

## Use them

<!-- snippet: examples/fonts/main.cpp tree -->
```cpp
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}})[
    lv::label({.style = {.text_font = title}, .text = "Wake up"}),
    lv::label({.style = {.text_font = numeral}, .text = "06:45"}),
    lv::label({.style = {.text_font = label}, .text = "Tomorrow, in 7 hours"})];
```

## A font without a token

A font that no token names is an exception to the design, and the code says so:

```cpp
constexpr auto odd = lv::font<barlow_ttf>(18 * px);

lv::label({.style = {.text_font = lv::raw(odd)}, .text = "18 px, once"})
```

Without `lv::raw` the build stops:

```text
error: static assertion failed: lv: wrong value for this property. It takes a font from a
typography token (`type(tok.text.title)` with an lv::typeface, see lv/font.hpp), or
lv::raw(font) for a font that no token names. You gave const _lv_font_t*.
```

## A size that changes while the program runs

A font made by the compiler has one size. Where the size is only known on the device, `lv::scaled_font` lets the device draw the glyphs. A style binds to it, as a property binds to a state:

<!-- snippet: examples/scaled_font/main.cpp font -->
```cpp
constinit lv::scaled_font reading{barlow_ttf, tok.text.label};  // the size of the token: 16 px

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "reading", .style = {.text_font = lv::bind(reading)}, .text = "21.5"})];
```

<!-- snippet: examples/scaled_font/main.cpp resize -->
```cpp
  reading.resize(tok.text.title);     // 28 px: the label follows
  reading.resize(lv::raw(31 * px));   // a size that no token names
```

- Its size comes from a typography token too. A size that no token names is `lv::raw(...)`.
- It needs LVGL's TinyTTF, which is off by default. Turn it on before lvgl26 is added: `set(LVGL26_TTF ON)`.
- The device pays for it: the whole font file in flash, TinyTTF's code, and heap for the glyphs it has drawn.
- A theme cannot hold such a font, because a theme is a constant.

## What a font costs

Barlow Medium with its 95 characters, measured once on one machine:

| Size | Compile time | In flash |
|---|---|---|
| 14 px | about 1 s | about 5 KB |
| 24 px | about 2 s | about 10 KB |
| 48 px | about 5 s | about 32 KB |

- The font file itself does not reach the program.
- Kerning pairs add three bytes each. Barlow Medium at 32 px has 748 among these characters.
- Make the fonts in one file, so that only that file pays the compile time.

## Rules and limits

- Sizes are from 4 px to 255 px, and a font holds at most 255 characters.
- There is no hinting. Very small sizes are a little softer than a font made with LVGL's converter.
- Kerning is read from the file's `kern` feature (GPOS). Kerning that differs by script is not read.
- OpenType files with CFF outlines (`.otf`) are refused with a message.
- A character the file lacks stops the build and names the character.
- lvgl26's CMake raises GCC's limit for compile-time work, because a large font needs it.
