# Fonts at compile time: research

Checked 2026-11-16. Marks follow `docs/research/README.md`.

`source` on a local path: the code was read directly.

`source` on a web link: the page was fetched and read through an extraction tool, not by eye.

`measured`: our own measurement; the row says how.

Local roots: `L96` is the LVGL v9.6.0 source tree, `L10` the tree of the v10 pin.

Already known (`lvgl-upstream.md`): run-time routes `lv_binfont_create_from_buffer`, `lv_tiny_ttf_create_data`, FreeType. Not repeated here.

## 1. LVGL built-in font format

### 1.1 What a constant font consists of

| Finding | Link | Mark |
|---|---|---|
| Four constant objects are needed: `lv_font_t`, `lv_font_fmt_txt_dsc_t`, a `lv_font_fmt_txt_glyph_dsc_t[]`, a `uint8_t[]` bitmap. Plus `lv_font_fmt_txt_cmap_t[]` and optional kerning. | `L96/include/lvgl/font/lv_font_fmt_txt.h` | source |
| `lv_font_t` needs: `get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt`, `get_glyph_bitmap = lv_font_get_bitmap_fmt_txt`, `dsc`, `line_height`, `base_line`. | `L96/include/lvgl/font/lv_font.h` | source |
| Other `lv_font_t` fields: `cap_height`, `x_height`, `subpx`, `kerning`, `static_bitmap`, `underline_position`, `underline_thickness`, `fallback`, `user_data`, `release_glyph`. | `L96/include/lvgl/font/lv_font.h` | source |
| `release_glyph` may stay null. LVGL checks it before the call. Built-in fonts leave it null. | `L96/src/font/lv_font.c:110` | source |
| `kerning` zero means `LV_FONT_KERNING_NORMAL`. `subpx` zero means `LV_FONT_SUBPX_NONE`. | `L96/include/lvgl/font/lv_font.h` | source |
| `base_line` is measured from the bottom of the line. Both values are whole pixels (`int32_t`). | `L96/include/lvgl/font/lv_font.h` | source |
| The built-in code casts `font->dsc` to a non-const pointer but does not write to it when `are_glyphs_dynamic_loaded` is false. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:97,234,300` | source |
| Both callbacks are public functions in `include/lvgl`. The private header is not needed for a plain font. | `L96/include/lvgl/font/lv_font_fmt_txt.h:227-239` | source |

### 1.2 `lv_font_fmt_txt_glyph_dsc_t`

| Finding | Link | Mark |
|---|---|---|
| Default layout (`LV_FONT_FMT_TXT_LARGE == 0`): `bitmap_index:20`, `adv_w:12`, `box_w:u8`, `box_h:u8`, `ofs_x:i8`, `ofs_y:i8`. Eight bytes. | `L96/include/lvgl/font/lv_font_fmt_txt.h:30-46` | source |
| Limits of that layout: bitmap under 1 MiB, advance under 256 px, box at most 255 px, offsets within -128..127. | same | source |
| `LV_FONT_FMT_TXT_LARGE` widens all fields. It is a build option, default 0. The struct layout changes with it. | `L96/lv_conf_template.h:1155` | source |
| `adv_w` is in 1/16 px. LVGL returns `(adv_w + kern + 8) >> 4`, a whole pixel. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:250-258` | source |
| `bitmap_index` is a byte offset into `glyph_bitmap`. Each glyph starts on a byte boundary. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:302` | source |
| Glyph id 0 is reserved. A lookup that yields 0 means "not found". The array starts with a zero entry. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:101,237`; `L96/src/font/lv_font_montserrat_14.c:1528` | source |
| `ofs_y` is the distance from the baseline up to the bottom edge of the box. The header comment ("from the top of the line") is wrong. | `L96/src/draw/lv_draw_label.c:671` (`y1 = pos.y + (line_height - base_line) - box_h - ofs_y`) | source |
| `ofs_x` is the distance from the pen position to the left edge of the box. | `L96/src/draw/lv_draw_label.c:669` | source |
| A glyph with `box_w * box_h == 0` (the space) returns no bitmap. Only its `adv_w` matters. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:118-119` | source |
| A tab is drawn as a space with doubled advance and box width. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:230-233,253,279` | source |

### 1.3 Character maps

| Finding | Link | Mark |
|---|---|---|
| A cmap covers `range_start .. range_start + range_length - 1`. `range_length` is 16 bits. | `L96/include/lvgl/font/lv_font_fmt_txt.h:61-111` | source |
| The cmaps are tried in order. The first range that contains the code point decides, even if it yields 0. Ranges must not overlap. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:339-379` | source |
| `FORMAT0_TINY`: both lists null. `glyph_id = glyph_id_start + (cp - range_start)`. Enough for a dense range such as U+0020..U+007E. | same | source |
| `FORMAT0_FULL`: `glyph_id_ofs_list` is `uint8_t[]`. A zero that is not at index 0 marks a missing character. | same, lines 348-355 | source |
| `SPARSE_TINY`: `unicode_list` is a sorted `uint16_t[]` of offsets from `range_start`. Binary search. `glyph_id = glyph_id_start + index`. | same, lines 356-365 | source |
| `SPARSE_FULL`: as above, then `glyph_id_ofs_list` is `uint16_t[]` indexed by the found position. | same, lines 366-376 | source |
| The `type` field must match the lists. The code branches on `type`, not on null pointers. | same | source |

### 1.4 Kerning

| Finding | Link | Mark |
|---|---|---|
| `kern_dsc` null means no kerning. `kern_classes` selects the type behind the pointer. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:240,393` | source |
| Applied value: `(int8 value * kern_scale) >> 4`, in 1/16 px, added to `adv_w` of the left glyph. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:250-256` | source |
| With `kern_scale = 16` a value is in 1/16 px and reaches about 8 px either way. | derived from the line above; `L96/src/font/lv_font_montserrat_14.c` uses 16 | source |
| Pairs (`lv_font_fmt_txt_kern_pair_t`): `glyph_ids` holds (left, right) pairs, sorted by left then right. Binary search. `values[i]` is `int8_t`. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:393-422` | source |
| `glyph_ids_size == 0`: ids are `uint8_t`. `== 1`: ids are `uint16_t`. | same | source |
| Classes (`lv_font_fmt_txt_kern_classes_t`): two `uint8_t` maps indexed by glyph id. Class 0 means none. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:427-438` | source |
| Class lookup: `class_pair_values[(left - 1) * right_class_cnt + (right - 1)]`. Constant time. | same | source |
| The class maps need one entry per glyph id, id 0 included. | same; `L96/src/font/lv_font_montserrat_14.c` | source |
| Kerning is looked up by glyph id of this font only. No kerning across a fallback font. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:240-245` | source |

### 1.5 Bitmaps, plain format

| Finding | Link | Mark |
|---|---|---|
| `bitmap_format`: 0 plain, 1 compressed with prefilter, 2 compressed without prefilter. | `L96/include/lvgl/font/lv_font_fmt_txt.h:146-150` | source |
| Plain accepts bpp 1, 2, 4 and 8 only. An assertion rejects others. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:125` | source |
| Bits are stored most significant first. At 4 bpp the high nibble is the left pixel. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:143-185` | source |
| `stride == 0`: rows are packed without padding. An odd-width 4 bpp row shares a byte with the next row. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:133-160` | source |
| `stride >= 1`: every row starts on a byte boundary. Row length is `ceil(w * bpp / 8)` rounded up to `stride`. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:264-273` | source |
| LVGL expands every glyph to A8 in a draw buffer: 4 bpp value times 17, 2 bpp times 85. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:58-68` | source |
| Exception: `static_bitmap == 1` and bpp 8. The software renderer blends straight from the constant array. No copy. | `L96/src/draw/sw/lv_draw_sw_letter.c:144-158` | source |
| In that path a stride of 0 is taken as the glyph width. | `L96/src/draw/sw/blend/lv_draw_sw_blend.c:111` | source |
| `static_bitmap = 1` is right for a plain font, 0 for a compressed one. | `L96/src/font/lv_font_montserrat_14.c:2212`; `lv_font_montserrat_28_compressed.c:3292` | source |
| Size of Montserrat at 4 bpp, 157 glyphs: 14 px 8832 B, 16 px 11090 B, 20 px 16986 B, 28 px 32484 B, 48 px 91912 B. | counted bytes in `L96/src/font/lv_font_montserrat_*.c` | measured |
| That is 56, 71, 108, 207 and 585 bytes per glyph. | derived from the row above | measured |

### 1.6 Bitmaps, compressed format

| Finding | Link | Mark |
|---|---|---|
| The decompressor is compiled only with `LV_USE_FONT_COMPRESSED`. Default is 0. | `L96/lv_conf_template.h:1158` | source |
| Without it, a compressed font logs a warning and draws nothing. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:212-215` | source |
| Compressed accepts bpp 2, 3 and 4 only. Not 1, not 8. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:483-496` | source |
| The stream is one run of `bpp`-bit pixels, most significant bit first, for the whole glyph. Rows are not aligned. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:547-597` | source |
| RLE: the first pixel is raw. A raw pixel equal to the one before it starts repeat mode. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:611-626` | source |
| Repeat mode: each 1 bit is one more repeat, up to ten. A 0 bit ends it; a raw pixel follows. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:627-653` | source |
| An eleventh 1 bit is followed by a 6-bit counter `c`. `c > 0` means `c` repeats in total, then a raw pixel. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:633-663` | source |
| `c == 0` means no further repeat: the raw pixel follows at once. Longest run is 75 pixels. | same; constants in [compress.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/compress.js) | source |
| Prefilter (format 1): each row is XORed with the row above before RLE. The first row is unchanged. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:519-527` | source |
| lv_font_conv states about 10 % extra gain from the XOR prefilter. | [font_spec.md](https://github.com/lvgl/lv_font_conv/blob/master/doc/font_spec.md) | source |
| The decoder allocates one or two row buffers with `lv_malloc` per glyph and keeps its state in a global. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:18,500-506` | source |
| Montserrat 28 px, 4 bpp: 32484 B plain, 17491 B compressed with prefilter. Ratio 0.54. | counted bytes in `lv_font_montserrat_28.c` and `lv_font_montserrat_28_compressed.c` | measured |
| v9.6.0 `get_bits` reads `in[byte_pos + 1]` when a read ends exactly on a byte boundary. One byte past the data. | `L96/src/font/fmt_txt/lv_font_fmt_txt.c:590` | source |

### 1.7 Difference in the v10 pin

| Finding | Link | Mark |
|---|---|---|
| `lv_font_fmt_txt.h` and the private header are identical in both versions. | `diff` of `L96` and `L10` | measured |
| `lv_font_fmt_txt.c` differs in one line: `bit_pos + len > 8` instead of `>= 8`. The over-read is fixed. | `L10/src/font/fmt_txt/lv_font_fmt_txt.c:590` | measured |
| `lv_font.h` differs in one line: `lv_font_info_t` gains `face_index`. `lv_font_t` is unchanged. | `L10/include/lvgl/font/lv_font.h:183` | measured |
| The v10 pin read was commit `b283a12e`, dated 2026-11-15. | `git log` in `L10` | measured |

## 2. What lv_font_conv does

| Finding | Link | Mark |
|---|---|---|
| It rasterises with a WebAssembly build of FreeType. Kerning and tables come from opentype.js. | [README](https://github.com/lvgl/lv_font_conv/blob/master/README.md), [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js) | source |
| Size: `FT_Set_Pixel_Sizes(face, 0, size)`. The em square is `size` pixels. Scale is `size / unitsPerEm`. | [freetype/index.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/freetype/index.js) | source |
| Load flags by default: `FT_LOAD_RENDER`, `FT_LOAD_FORCE_AUTOHINT`, `FT_LOAD_TARGET_LIGHT`. | same | source |
| So the font's own TrueType instructions are not run. The FreeType auto-hinter is used. | same | source |
| Light hinting snaps glyphs to the pixel grid on the Y axis only. Horizontal shapes are untouched. | [FreeType reference](https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html) | source |
| `--autohint-off` disables the auto-hinter. `--autohint-strong` uses `FT_LOAD_TARGET_NORMAL` and "will break kerning". | [README](https://github.com/lvgl/lv_font_conv/blob/master/README.md), [freetype/index.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/freetype/index.js) | source |
| `box_w`, `box_h` are the FreeType bitmap `width` and `rows`. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js) | source |
| `ofs_x = bitmap_left`. `ofs_y = bitmap_top - rows`. | same | source |
| `adv_w = round(linearHoriAdvance * 16)`. That is the unhinted advance, kept fractional. | same; [lv_table_glyf.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/writers/lvgl/lv_table_glyf.js) | source |
| Pixels are 8-bit coverage reduced by `p >>> (8 - bpp)`. Truncation, no rounding. | [table_glyf.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/table_glyf.js) | source |
| `ascent = ceil(ascender * scale)`, `descent = floor(descender * scale)`, from the FreeType face. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js) | source |
| `line_height = ascent - descent`. `base_line = -descent`. The line gap is not used. | [lv_table_head.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/writers/lvgl/lv_table_head.js) | source |
| `cap_height`, `x_height` come from OS/2, scaled and rounded. Underline values come from `post`, scaled and rounded. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js) | source |
| Kerning: opentype.js `getKerningValue`. It uses GPOS if present, otherwise the `kern` table. | [opentype.js font.mjs](https://github.com/opentypejs/opentype.js/blob/master/src/font.mjs) | source |
| Kerning in pixels: `value * size / unitsPerEm`. Stored as `round(px / kerningScale * 16)` in one signed byte. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js), [font.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/font.js) | source |
| `kerningScale` is 1.0 unless the largest value is 7.5 px or more. Then `ceil(max / 7.5 * 16) / 16`. | [font.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/font.js) | source |
| `kern_scale` in C is `round(kerningScale * 16)`. So 16 in the usual case. | [lv_table_head.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/writers/lvgl/lv_table_head.js) | source |
| Classes are chosen when not larger than pairs, or always with `--force-fast-kern-format`. At most 254 classes per side. | [table_kern.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/table_kern.js) | source |
| A left class is the set of glyphs with an identical kerning row. Right classes likewise by column. | same | source |
| Subsetting: only the requested ranges and symbols are rendered. Ids run from 1 in code point order. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js), [font.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/font.js) | source |
| Kerning is kept only between glyphs of the chosen set and of the same source font. | [collect_font_data.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/collect_font_data.js) | source |
| Compression is on by default, except at 1 bpp. `static_bitmap` is 1 only when uncompressed. | [table_glyf.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/font/table_glyf.js), [lv_table_head.js](https://github.com/lvgl/lv_font_conv/blob/master/lib/writers/lvgl/lv_table_head.js) | source |
| `--stride` needs `--no-compress`, bpp 1, 2, 4 or 8, and LVGL 9.3 or newer. | [README](https://github.com/lvgl/lv_font_conv/blob/master/README.md) | source |

## 3. The minimum of TrueType

### 3.1 Barlow

| Finding | Link | Mark |
|---|---|---|
| The repository's `Barlow-Medium.ttf`, `BarlowCondensed-SemiBold.ttf`, `BarlowCondensed-Bold.ttf` have sfnt version `0x00010000` and a `glyf` table. No `CFF`. | `site/media/fonts/*.ttf`, read by a table-directory script | measured |
| They have no `kern` table. Kerning is in GPOS only, feature `kern`. | same | measured |
| The `kern` feature uses lookup type 2 only: one PairPosFormat1 subtable, then two to five PairPosFormat2. No extension lookups. | same | measured |
| Every subtable has `valueFormat1 = 0x0004` (X advance) and `valueFormat2 = 0`. | same | measured |
| `unitsPerEm` 1000. `indexToLocFormat` 0 (short offsets). 694 glyphs. | same | measured |
| `cmap` has two subtables, (0,3) and (3,1), both format 4. No format 12. | same | measured |
| `hhea` ascender 1000, descender -200, line gap 0. OS/2 typo values are the same. | same | measured |
| Cap height 700. x-height 509 to 514 by weight. Underline position -75, thickness 50. | same | measured |
| ASCII contains composite glyphs: `.`, `:`, `=` and the grave accent. | same | measured |
| In U+00A0..U+017F, 160 of the glyphs are composites (accented letters). | same | measured |
| Largest simple glyph in U+0020..U+017F: 112 points, 5 contours. `maxComponentDepth` says 4. | same | measured |
| The files carry TrueType instructions (`fpgm`, `prep`, `cvt`). A rasteriser without hinting skips them. | same | measured |
| Files are 103 to 110 kB. `glyf` is 66 to 73 kB. | same | measured |
| Licence file next to the fonts is `OFL.txt`. | `site/media/fonts/OFL.txt` | measured |

### 3.2 Tables

All values are big-endian.

| Finding | Link | Mark |
|---|---|---|
| Table directory: 12-byte header (`numTables` at 4), then 16-byte records: tag, checksum, offset, length. | [OpenType font file](https://learn.microsoft.com/en-us/typography/opentype/spec/otff) | measured (the script read Barlow this way) |
| `head`: `unitsPerEm` u16 at 18. `indexToLocFormat` i16 at 50. | [head](https://learn.microsoft.com/en-us/typography/opentype/spec/head) | measured |
| `maxp`: `numGlyphs` u16 at 4. | [maxp](https://learn.microsoft.com/en-us/typography/opentype/spec/maxp) | measured |
| `hhea`: ascender i16 at 4, descender at 6, lineGap at 8, `numberOfHMetrics` u16 at 34. | [hhea](https://learn.microsoft.com/en-us/typography/opentype/spec/hhea) | measured |
| `hmtx`: `numberOfHMetrics` records of (advanceWidth u16, lsb i16). Later glyphs reuse the last advance. | [hmtx](https://learn.microsoft.com/en-us/typography/opentype/spec/hmtx) | recalled |
| `loca`: `numGlyphs + 1` offsets into `glyf`. Short format stores offset / 2 as u16. Long stores u32. | [loca](https://learn.microsoft.com/en-us/typography/opentype/spec/loca) | measured |
| Two equal consecutive `loca` offsets mean an empty glyph (the space). | [glyf](https://learn.microsoft.com/en-us/typography/opentype/spec/glyf) | source |
| `OS/2`: `sxHeight` i16 at 86, `sCapHeight` at 88, from version 2. | [OS/2](https://learn.microsoft.com/en-us/typography/opentype/spec/os2) | measured |
| `post`: `underlinePosition` i16 at 8, `underlineThickness` at 10. | [post](https://learn.microsoft.com/en-us/typography/opentype/spec/post) | measured |
| `cmap`: pick the subtable (3,10) or (0,4) with format 12 if present, else (3,1) or (0,3) with format 4. | [cmap](https://learn.microsoft.com/en-us/typography/opentype/spec/cmap) | source |
| Format 4: find the first segment with `endCode >= c`. If `startCode > c` the glyph is 0. | same | source |
| Format 4, `idRangeOffset == 0`: `glyph = (c + idDelta) mod 65536`. | same | source |
| Format 4, otherwise: read u16 at `&idRangeOffset[i] + idRangeOffset[i] + 2 * (c - startCode[i])`. If not 0, add `idDelta` mod 65536. | same | source |
| Format 4 arrays start at offset 14: `endCode[n]`, 2 bytes pad, `startCode[n]`, `idDelta[n]`, `idRangeOffset[n]`. `n = segCountX2 / 2` at offset 6. | same | source |
| Format 12: `numGroups` u32 at 12, then groups of (startCharCode, endCharCode, startGlyphID), all u32, sorted. | same | source |
| Format 12: `glyph = startGlyphID + (c - startCharCode)`. | same | source |
| `kern` table, version 0: `nTables` at 2. Format 0 subtable: `nPairs` at 6 of the subtable, then (left u16, right u16, value i16) sorted. | [kern](https://learn.microsoft.com/en-us/typography/opentype/spec/kern) | recalled |
| GPOS header: `scriptListOffset` at 4, `featureListOffset` at 6, `lookupListOffset` at 8. | [GPOS](https://learn.microsoft.com/en-us/typography/opentype/spec/gpos) | source |
| Pair adjustment is lookup type 2. Kerning changes `xAdvance` of the first glyph, in font units. | same | source |
| PairPosFormat1: coverage, `valueFormat1`, `valueFormat2`, then one PairSet per covered first glyph, sorted by second glyph. | same | source |
| PairPosFormat2: coverage, two ClassDef tables, then a `class1Count` by `class2Count` matrix of value records. | same | source |
| A value record holds one i16 per set bit of the format. `0x0004` is X advance. | same | source |
| Lookup type 9 wraps another type behind a 32-bit offset. | same | source |
| Subtables of one lookup are tried in order. The first whose coverage holds the left glyph decides. | [GPOS](https://learn.microsoft.com/en-us/typography/opentype/spec/gpos) | recalled |
| Coverage and ClassDef formats 1 and 2 are defined in the common table chapter. | [chapter 2](https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2) | recalled |

### 3.3 Glyph outlines

| Finding | Link | Mark |
|---|---|---|
| Glyph header: `numberOfContours` i16, then `xMin`, `yMin`, `xMax`, `yMax` i16. Negative count means composite. | [glyf](https://learn.microsoft.com/en-us/typography/opentype/spec/glyf) | source |
| Simple glyph: `endPtsOfContours[n]`, `instructionLength`, instructions, flags, x deltas, y deltas. | same | source |
| Point count is the last `endPtsOfContours` value plus 1. | same | source |
| Flags: `0x01` on curve, `0x02` x is one byte, `0x04` y is one byte, `0x08` repeat, `0x10` and `0x20` sign or same. | same | source |
| Repeat: the next byte says how many more times the flag applies. | same | source |
| One-byte coordinate: unsigned, positive if the `0x10`/`0x20` bit is set, else negative. | same | source |
| Two-byte case: if the `0x10`/`0x20` bit is set the delta is zero and no bytes are stored. Else a signed i16. | same | source |
| Coordinates are deltas from the previous point. The first is relative to (0,0). | same | source |
| The bounding box in the header contains all control points. It may be loose but never too small. | same | source |
| Composite: records of flags u16, glyph index u16, two arguments, optional transform. Loop while `MORE_COMPONENTS` (0x0020). | same | source |
| `ARG_1_AND_2_ARE_WORDS` (0x0001): arguments are 16 bit, else 8 bit. `ARGS_ARE_XY_VALUES` (0x0002): they are a signed offset. | same | source |
| Transform: `WE_HAVE_A_SCALE` 0x0008 (one F2DOT14), `X_AND_Y_SCALE` 0x0040 (two), `TWO_BY_TWO` 0x0080 (four). | same | source |
| `x' = xscale * x + scale10 * y`, `y' = scale01 * x + yscale * y`. Then add the offset. | same | source |
| If neither offset flag is set, the offset is not scaled. That is the recommended default. | same | source |
| `USE_MY_METRICS` (0x0200): the composite takes advance and side bearings from that component. | same | source |
| Without `ARGS_ARE_XY_VALUES` the arguments are point numbers to align. Rare. | same | source |
| Composites may nest. The graph is acyclic. | same | source |
| Two consecutive off-curve points imply an on-curve point at their midpoint. | [TrueType reference manual, chapter 1](https://developer.apple.com/fonts/TrueType-Reference-Manual/RM01/Chap1.html) | recalled |
| Each contour is closed. A contour may start with an off-curve point. | same | recalled |
| The fill rule is non-zero winding. | [TrueType reference manual, chapter 2](https://developer.apple.com/fonts/TrueType-Reference-Manual/RM02/Chap2.html) | recalled |
| Contours and components may overlap. The `OVERLAP_SIMPLE` and `OVERLAP_COMPOUND` flags are optional hints. | [glyf](https://learn.microsoft.com/en-us/typography/opentype/spec/glyf) | source |

### 3.4 The coverage algorithm (font-rs, ab_glyph_rasterizer)

| Finding | Link | Mark |
|---|---|---|
| The algorithm keeps one float per pixel plus a few spare cells. Each line segment adds signed area to the cells it crosses. | [ab_glyph_rasterizer raster.rs](https://github.com/alexheretic/ab-glyph/blob/main/rasterizer/src/raster.rs) | source |
| A final running sum over the buffer gives coverage. `abs` of the sum is the alpha. The caller clamps to 1. | same | source |
| No edge list, no sorting, no active-edge table. Segments can be fed in any order. | same; [stb_truetype v2 article](https://nothings.org/gamedev/rasterize/) | source |
| The code is adapted from font-rs (Apache 2.0, Google 2015). The notice must be kept. | same file, header | source |
| stb_truetype v2 uses the same idea per scanline. It computes exact area coverage, a box filter. | [stb_truetype v2 article](https://nothings.org/gamedev/rasterize/) | source |
| stb v1 oversampled 5 or 15 times vertically. v2 does one pass per row and is slightly faster. | same | source |
| Known limit: overlapping contours add their areas. Clamping hides it inside, but overlap edges come out too dark. | same | source |
| LVGL's own Tiny TTF uses stb_truetype with rasteriser version 2. | `L96/src/libs/tiny_ttf/stb_truetype_htcw.h:1179` | source |

Pseudocode, from the source above. `a` has `w * h + 4` cells, all zero. Points are in pixels, y down, origin at the box corner.

```
draw_line(p0, p1):
    if |p0.y - p1.y| <= epsilon: return          # horizontal adds nothing
    dir = +1 if p0.y < p1.y else -1; if dir < 0: swap(p0, p1)
    dxdy = (p1.x - p0.x) / (p1.y - p0.y)
    x = p0.x;  if p0.y < 0: x -= p0.y * dxdy
    for y in max(0, floor(p0.y)) .. min(h, ceil(p1.y)) - 1:
        row = y * w
        dy = min(y + 1, p1.y) - max(y, p0.y)
        xnext = x + dxdy * dy
        d = dy * dir
        x0, x1 = min(x, xnext), max(x, xnext)
        x0i = floor(x0);  x1i = ceil(x1)
        if x1i <= x0i + 1:                       # within one pixel column
            xmf = 0.5 * (x + xnext) - x0i
            a[row + x0i]     += d - d * xmf
            a[row + x0i + 1] += d * xmf
        else:
            s = 1 / (x1 - x0)
            x0f = x0 - x0i
            a0 = 0.5 * s * (1 - x0f)^2
            x1f = x1 - x1i + 1
            am = 0.5 * s * x1f^2
            a[row + x0i] += d * a0
            if x1i == x0i + 2:
                a[row + x0i + 1] += d * (1 - a0 - am)
            else:
                a1 = s * (1.5 - x0f)
                a[row + x0i + 1] += d * (a1 - a0)
                for xi in x0i + 2 .. x1i - 2: a[row + xi] += d * s
                a2 = a1 + (x1i - x0i - 3) * s
                a[row + x1i - 1] += d * (1 - a2 - am)
            a[row + x1i] += d * am
        x = xnext

draw_quad(p0, p1, p2):
    dev = p0 - 2 * p1 + p2;  devsq = dev.x^2 + dev.y^2
    if devsq < 0.333: draw_line(p0, p2); return
    n = 1 + floor((3 * devsq)^(1/4))             # number of line pieces
    p = p0
    for i in 1 .. n - 1:
        t = i / n
        q = lerp(lerp(p0, p1, t), lerp(p1, p2, t), t)
        draw_line(p, q);  p = q
    draw_line(p, p2)

finish:
    acc = 0
    for i in 0 .. w * h - 1:
        acc += a[i]
        alpha[i] = min(|acc|, 1)
```

| Finding | Link | Mark |
|---|---|---|
| Writes outside the buffer are dropped. The upstream code skips the rest of that row. | [raster.rs](https://github.com/alexheretic/ab-glyph/blob/main/rasterizer/src/raster.rs) | source |
| The running sum is not reset per row. A closed outline inside the box sums to zero at each row end. | same | source |
| The fourth root needs `sqrt` twice. No other library maths is needed. | same | source |
| Work is about (line pieces times rows crossed) plus `w * h` for the final pass. No oversampling. | derived from the pseudocode | source |

## 4. SDF and MSDF

| Finding | Link | Mark |
|---|---|---|
| msdfgen makes distance fields from outlines: `sdf` and `psdf` (1 channel), `msdf` (3 channels), `mtsdf` (4 channels). | [msdfgen README](https://github.com/Chlumsky/msdfgen/blob/master/README.md) | source |
| Three channels let sharp corners survive. A single channel always rounds them. | same | source |
| Rendering per output pixel: sample the texture with interpolation, take the median of three, scale, clamp. | same (the shader in the README) | source |
| The README's examples use 32 by 32 cells and a range of 4 pixels. | same | source |
| The screen pixel range must not fall under 1. Under 2, anti-aliasing "will likely fail". | same | source |
| The core library has no dependencies. Font loading uses FreeType. | same | source |
| The thesis measured rendering on a desktop GPU (Nvidia GTX 760) only. | [thesis](https://github.com/Chlumsky/msdfgen/files/3050967/thesis.pdf), p. 61 | source |
| There, MSDF rendering was 12 to 56 % slower than single-channel SDF. Texture sampling dominated. | same, p. 61 and 63 | source |
| A stroke thinner than about two cells of the field cannot be encoded properly. | same, p. 64 | source |
| The author sees the biggest use in text in real-time 3D scenes. | same, p. 63 | source |
| LVGL's glyph formats are A1, A2, A3, A4, A8, IMAGE, VECTOR, SVG, CUSTOM. There is no SDF format. | `L96/include/lvgl/font/lv_font.h:35-51` | source |
| The software renderer handles only those. No distance-to-alpha step exists. | `L96/src/draw/sw/lv_draw_sw_letter.c:120-200` | source |
| "SDF" appears in LVGL's `src`, `include` and `docs` only inside the bundled stb_truetype header. | `grep` over `L96` | measured |
| Slint's software renderer can embed fonts as SDF. It is opt-in (`sdf-fonts`). | [Slint font handling](https://docs.slint.dev/latest/docs/slint/guide/development/fonts/) | source |
| Slint states the purpose: smaller binaries on small flash, "trading some rendering quality". | same | source |
| Slint's default is plain: glyphs pre-rendered at each pixel size the UI uses. | same | source |
| Slint had to tune glyph centring, advance rounding and the threshold after adding SDF. | [Slint PR 6868](https://github.com/slint-ui/slint/pull/6868) | source |
| Flash, 8-bit cells of 32 by 32: SDF 1024 B per glyph, MSDF 3072 B per glyph. | arithmetic | measured |
| Flash, 4 bpp bitmaps: 56 B at 14 px, 71 B at 16 px, 108 B at 20 px, 207 B at 28 px, 585 B at 48 px. | section 1.5 | measured |
| Four bitmap sizes (14, 16, 20, 28) cost 442 B per glyph. Less than half of one SDF cell. | arithmetic on the rows above | measured |
| CPU, bitmap: one table lookup per pixel, or none at 8 bpp with a static bitmap. | section 1.5 | source |
| CPU, MSDF in software: four texel reads and three interpolations per channel, a median and a clamp, per pixel. | derived from the README shader | source |

Conclusion: MSDF does not fit a software-rendered MCU UI with a few fixed sizes.

It costs more flash than the bitmaps it replaces, more CPU per pixel, and LVGL has no draw path for it.

It loses quality at small sizes: no hinting, and thin strokes need a finer field.

It fits where one asset must serve many or animated sizes, rotation, outlines or glow, and a GPU samples textures.

For lvgl26 that means a GPU draw unit, not the `lv_font_fmt_txt` route.

The only MCU precedent found is Slint, with single-channel SDF, as a flash saving for many sizes.

## 5. Prior art

| Finding | Link | Mark |
|---|---|---|
| MCUFont: encoder on the PC (C++, FreeType, TTF and BDF in), decoder in C on the target. | [mcufont](https://github.com/mcufont/mcufont) | source |
| MCUFont: 16 grey levels, kerning, decoder code 1 to 5 kB, no run-time scaling. | same | source |
| MCUFont claims a "high compression ratio" but its README gives no number. | same | source |
| MCUFont's format combines run-length and dictionary coding. | [uGFX mf_rlefont.h](https://api.ugfx.io/src_2gdisp_2mcufont_2mf__rlefont_8h_source.html) | summary |
| LVGL's own RLE with XOR prefilter: 0.54 of plain at 28 px, 4 bpp. | section 1.6 | measured |
| A CCITT G4 style font codec reports 5:1 to 20:1. Not comparable: a different format and likely 1 bpp. | [blog post](https://bitbanksoftware.blogspot.com/2024/08/compressed-fonts-yes-please.html) | summary |
| `packed-font` (Rust): a macro pre-renders a TTF at build time. Used like `include_bytes!`. No external tool. | [packed-font README](https://docs.rs/crate/packed-font/0.1.0/source/README.md) | source |
| `packed-font`: skrifa and tiny-skia, full hinting, 4 bpp, a modified RLE, ASCII plus the degree sign. | same | source |
| `packed-font`: a 72 px font fits under 16 kB of ROM; small fonts about 4 kB. Compression helps less at small sizes. | same | source |
| `packed-font` runs ordinary Rust at build time. It is not constant evaluation by the compiler's interpreter. | same | source |
| Slint pre-renders glyphs in its own compiler and embeds them in the binary. | [Slint font handling](https://docs.slint.dev/latest/docs/slint/guide/development/fonts/) | source |
| `wttf`: a C++17 TrueType parser and rasteriser, run time, no bounds checks. | [wttf](https://github.com/svuolli/wttf) | summary |

## 6. GCC constant-evaluation limits

| Finding | Link | Mark |
|---|---|---|
| `-fconstexpr-ops-limit`: operations in "a single constexpr evaluation". Default 33554432 (2^25). | [GCC C++ dialect options](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html) | source |
| `-fconstexpr-loop-limit`: iterations of one loop. Default 262144 (2^18). | same | source |
| `-fconstexpr-depth`: nesting depth of calls. The standard's minimum is 512. | same | source |
| `-fconstexpr-cache-depth`: default 8. Trades speed against memory. | same | source |
| Division by zero in floating point is not a constant expression unless `-fconstexpr-fp-except`. | same | source |
| The GCC page says nothing about `#embed` limits or cost per operation. | same | source |
| Our printer demo needs `-fconstexpr-ops-limit=20000000000` and about 176 s. | `docs/research/cpp26.md` line 51 | measured (2026-11-15, earlier note) |
| GCC memoises constant evaluations. Clang was about 2 times faster than GCC on one test without memoisation. | [cfe-dev, March 2021](https://lists.llvm.org/pipermail/cfe-dev/2021-March/067812.html) | summary |

## Our spike: a TTF rasterised by the compiler

`spikes/font_from_ttf.hpp` reads a TrueType file in constant evaluation and returns a constant `lv_font_t`. GCC 16.2.0, macOS arm64, 2026-11-16. Barlow Medium, U+0020..U+007E, 4 bpp, plain format, one run each.

| Finding | How | Mark |
|---|---|---|
| LVGL draws the fonts: 14, 20, 32 and 72 px were rendered and looked at. | `spikes/font_from_ttf.cpp`, a PNG of the screen | `measured` |
| Extra compile time: 14 px 1.2 s, 24 px 1.9 s, 48 px 4.9 s. | `/usr/bin/time` on `cc1plus`, against a file without a font (0.4 s) | `measured` |
| In instructions: 17e9, 28e9 and 74e9 over the 5e9 of the empty file. | `/usr/bin/time -l` on `cc1plus` | `measured` |
| Font data in the object file: 14 px 5.0 KB, 24 px 10.1 KB, 48 px 31.8 KB. The 103 KB file does not reach it. | `size -m` at `-Os` | `measured` |
| LVGL's Montserrat 14, with more characters and kerning, is 13.7 KB. | `size -m` on its object file | `measured` |
| The whole font as one evaluation passes GCC's default operation limit from about 24 px. 100 million is enough for 48 px. | `-fconstexpr-ops-limit` raised step by step | `measured` |

The spike reads no kerning and applies no hinting. It follows font-rs closely: see rule 17 below.

## Not confirmed

- No C++ `constexpr` TrueType parser or rasteriser was found. One search only; absence is not proven.
- MCUFont compression ratios: no number found. Its format document could not be fetched.
- Whether LVGL has an open issue or plan for SDF fonts. The GitHub search failed in the sandbox.
- Cost of one GCC constexpr operation, and how GCC counts operations. No source found.
- Whether GCC evaluates floating point in constant expressions slower than integers. No source found.
- Whether Barlow's glyphs have overlapping contours. Not checked; the flag is optional.
- Whether the Barlow files in the repository are byte-identical to the Google Fonts release.
- Which GPOS script and language opentype.js uses for kerning. Its `position.mjs` was not read.
- The implied on-curve point rule and the non-zero rule are `recalled`. Read the Apple chapters before relying on wording.
- The `kern` format 0 layout and the `hmtx` layout are `recalled`. Barlow does not need `kern`.
- The lv_font_conv encoder's exact bit sequence. The LVGL decoder was read instead; it is the authority.
- TouchGFX, Qt for MCUs and others: no source read on whether any offers SDF fonts.
- The MSDF software cost is derived from the shader, not measured on an MCU.

## What it means for us

1. Emit the plain format first: `bitmap_format = 0`, `static_bitmap = 1`. It needs no LVGL build option.
2. Offer 4 bpp (small) and 8 bpp (zero-copy blend in the software renderer). Both are valid plain formats.
3. Use `stride = 0` and pack rows without padding at 4 bpp. Start each glyph on a byte boundary.
4. Put a zero entry at glyph id 0. Number glyphs from 1 in code point order.
5. One `FORMAT0_TINY` cmap covers U+0020..U+007E. Add `SPARSE_TINY` for scattered characters. Never overlap ranges.
6. Compute `adv_w = round(advanceWidth * 16 * px / unitsPerEm)`. Reject at compile time above 4095 or boxes above 255.
7. Compute `ofs_y` as the bottom of the box above the baseline. Ignore the header comment.
8. Match lv_font_conv metrics: `line_height = ceil(asc * s) - floor(desc * s)`, `base_line = -floor(desc * s)`, from `hhea`.
9. Take the box from the glyph header: floor of the scaled minimum, ceil of the scaled maximum.
10. Barlow needs GPOS, not `kern`. Read lookup type 2, formats 1 and 2, X advance of the first glyph.
11. Emit kerning as classes with `kern_scale = 16`. Lookup is constant time. Reject values outside the int8 range.
12. Composite glyphs are required even for ASCII (`.`, `:`, `=`). Support offsets and the three scale forms. Recurse.
13. Skip instructions. We get no hinting; lv_font_conv gets vertical-only auto-hinting. Expect slightly softer horizontal stems.
14. Soften that loss by size choice: Barlow's cap height is 0.7 em, so sizes of 10, 20, 30 px land on whole pixels.
15. Use the font-rs accumulation algorithm. It needs no sorting and no oversampling, which suits a step budget.
16. Round coverage to 4 bits instead of truncating as lv_font_conv does. Full coverage must give 15.
17. Keep the Apache 2.0 notice if the code follows font-rs or ab_glyph_rasterizer closely.
18. The ops limit is per evaluation. Rasterise each glyph in its own constant so no single evaluation is large.
19. Keep every loop under 262144 iterations. A byte loop over a 110 kB font is fine; over a 300 kB font is not.
20. Compressed output is a second step. It needs `LV_USE_FONT_COMPRESSED`, heap at run time, and halves the bitmap.
21. If compressed is added for v9.6.0, append one spare byte to the bitmap array. The v10 pin does not need it.
22. Test the encoder by round trip through LVGL's own decoder at run time, on both pins.
23. Do not pursue MSDF for this route. Revisit only with a GPU draw unit and a need for free scaling.
24. `.otf` with CFF outlines stays out of scope. Detect sfnt version `OTTO` and fail with a clear message.
25. Measure before claiming: compile time with `/usr/bin/time`, pixels against lv_font_conv output for the same size.

## Extra characters on top of the default set (2026-11-27)

Marks: `source` is read in this repository, `measured` is a probe compiled here (macOS, GCC 16.2, Barlow Medium at 16 px, `tools/compile_report.py` instructions and `size -m` of the object file), `reasoned` is mine.

### The problem

A drawn font has the characters from the space to the tilde when `characters` is empty, and only the characters named when it is not (`make_font`, `characters_of` in `include/lv/font.hpp`). `source`. So a price "2.20 €" or a line "4 of 5 · Strong" needs the euro sign and the middle dot, and naming them means typing all 95 ASCII characters again beside them. The coffee steps need both.

### What it costs for each extra glyph

| Characters | Compile time (instructions) | Constant data in the object file |
|---|---:|---:|
| 95 (the default) | 63.95 G | 110,361 bytes |
| 95 + 2 (`€` and `·`) | 65.50 G (+1.55 G, +2.4%) | 110,601 bytes (+240) |
| 95 + 10 (`€ · é è ä ö ü ß ° ×`) | 71.26 G (+7.3 G, +11.4%) | 111,599 bytes (+1,238) |

`measured`. An extra glyph costs about 0.75 G instructions, the same as one of the 95 (0.67 G each), and about 120 bytes of flash at 16 px, depending on its shape: its bitmap, its entry in the glyph table and 2 bytes in the character list.

Two facts that do not depend on how many are added, `source` and `reasoned`:

- The default characters are one range, and the character map of the font is the tiny form that needs no list. The first character outside the range, such as U+20AC, turns it into the sparse form: a list of 2 bytes for each character of the font, the 95 as well, about 200 bytes once. LVGL then finds a glyph by binary search over the list and not by an offset: a few more steps for each drawn character at run time. `stored_cmap` already does this for any non-contiguous set.
- The limits stay: a font holds at most 255 characters (`make_font`), and the list takes the distance from the first character in 16 bits, so characters up to U+FFFF (emoji are out, as today).

### The API: three forms

All three give a font of the default set plus the characters named.

| | A. A named constant of the set | B. A member of the typeface entry | C. A marker in the string |
|---|---|---|---|
| Typeface | `{"Barlow", 500, barlow_ttf, lv::ascii_plus<"€·">}` | `{"Barlow", 500, barlow_ttf, {}, "€·"}` | `{"Barlow", 500, barlow_ttf, "+€·"}` |
| One font | `lv::font<ttf>(16 * px, lv::ascii_plus<"€·">)` | `lv::font<ttf>(16 * px, {}, "€·")` | `lv::font<ttf>(16 * px, "+€·")` |
| Change to `face_file` | none: `characters` is already a view that a constant gives | a fifth member and its constructor | a rule in `characters_of` |
| Reads as | the set, then the extras, in one word | two strings, one of them empty | a convention hidden in a character |
| A mistake | a character the file lacks is the existing message, naming it | the same | a text that starts with `+` and means just that, no way out |
| Compose with other sets | yes: `lv::ascii_plus<"€·">` is a view; a Latin-1 preset would be another constant | no | no |

My recommendation is A: it changes no type, it is a constant like `lv::glyphs<...>` of the icons (`include/lv/icons.hpp`), and it also serves `lv::font<ttf>(size, characters)` and the split form (`lv::draw<"text.label">(type)`), which take the same `typeface`. `ascii_plus<"€·">` is a variable template of a fixed string, evaluated once and as a static string. The compile-fail test is a character that the file lacks, and a text that is not valid UTF-8.

### What I cannot settle

1. The name: `lv::ascii_plus<"...">` (my choice), `lv::ascii_and<"...">`, or `lv::with_ascii<"...">`.
2. Is a preset worth having beside it? `lv::latin1` would be the default set and U+00A0..U+00FF, 191 characters in all: it costs 96 more glyphs (about 72 G instructions and 12 KB at 16 px), which a project pays on purpose. The coffee steps need only `€` and `·`.
3. Should the sparse list be avoided for the default characters by sorting the extras into a second range (a second character map for U+20AC and the rest), so that the default range stays direct? It saves about 200 bytes and the binary search at run time, and costs a second map in LVGL's table (`cmap_num = 2`). I would not build it now.
4. `lv::scaled_font` (the device draws the glyphs) takes a TrueType file and draws any character, so nothing changes there. Is that right for the owner's list?

### Tests and checks to write with it

- `font`: the font of `lv::ascii_plus<"€·">` has the 95 and both glyphs (the glyph descriptor of U+20AC and U+00B7 is found, with a width); a character twice is one; the order of the extras does not matter.
- A test that the sparse form is used only when an extra is named: the default font stays on the tiny form.
- Compile-fail: a character the file lacks names it; an extra that makes more than 255 characters says so.
- The docs page `docs/fonts.md` gets the section, with the cost table above as ranges.
