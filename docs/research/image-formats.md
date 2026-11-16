# Image formats: what to accept as source, what to store in flash

Checked 2026-11-16. Question: which formats should the compiler decode (source), and which form should sit in flash when pixels are not stored raw.

This note builds on `docs/research/images.md`. It does not repeat LVGL's raw formats, its RLE and LZ4, lodepng, tjpgd, or the choices of Slint, Qt for MCUs and TouchGFX.

## How the marks were earned

- `source`: the linked page was fetched and read through a fetch tool that extracts text. Numbers are as the tool returned them.
- `measured`: our own count. Line counts are `curl <raw file> | wc -l` on 2026-11-16, comments and blank lines included.
- `summary`: only a search engine's summary of the page was seen.
- `recalled`: from memory. All of these are under "Not confirmed".
- A cell that says "derived" is arithmetic on a number from the linked page.

## 1. Web and modern formats

### 1.1 Compression ratio

| Finding | Link | Mark |
|---|---|---|
| QOI benchmark, 512-pixel icons, share of raw size: libpng 5.0%, stb 6.8%, QOI 8.4%. QOI is 1.7 times libpng there. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| 64-pixel icons: libpng 23.6%, stb 27.9%, QOI 28.7%. Small icons compress badly in every format; QOI is 1.2 times libpng. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| Web screenshots (flat UI): libpng 7.6%, QOI 8.3%. Cut-out objects with alpha (`pngimg`): libpng 17.0%, QOI 20.3%. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| Photos (Kodak): libpng 56.3%, QOI 58.3%. No lossless format gets a photo much below half its raw size. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| All 13 categories together: libpng 24.2%, stb 34.2%, QOI 28.2%. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| Size relative to QOIR on the same suite: WebP lossless 0.654, JPEG XL lossless (effort 7) 0.613, libpng 0.960, QOI 1.118, fpng 1.234, LZ4 over PNG filters 1.403. | [QOIR README](https://github.com/nigeltao/qoir) | `source` |
| So WebP lossless is about 0.68 of libpng, and JPEG XL lossless about 0.64 (derived from the row above). | [QOIR README](https://github.com/nigeltao/qoir) | `source` |
| Google's own study, 12,000 web PNGs: PNG 6.85 bpp, ZopfliPNG 5.05, WebP lossless 4.04. WebP lossy with alpha: 1.92 bpp. | [WebP lossless and alpha study](https://developers.google.com/speed/webp/docs/webp_lossless_alpha_study) | `source` |
| WebP lossy is 25% to 34% smaller than JPEG at equal SSIM, on Kodak, Tecnick and a web crawl. | [WebP compression study](https://developers.google.com/speed/webp/docs/webp_study) | `source` |
| For lossless non-photo images, WebP beats PNG by a narrow margin. HEIC and AVIF are sometimes behind PNG. | [Cloudinary: next-gen codecs](https://cloudinary.com/blog/time_for_next_gen_codecs_to_dethrone_jpeg) | `source` |
| For lossy non-photo images, AVIF and HEIC beat JPEG. AVIF leads at low fidelity, JPEG XL at medium to high fidelity. | [Cloudinary: next-gen codecs](https://cloudinary.com/blog/time_for_next_gen_codecs_to_dethrone_jpeg) | `source` |
| Plain RLE on an embedded UI, uncompressed / RLE / PNG: 82.9 / 23.2 / 6.7 kB flat, 82.9 / 72.6 / 27.5 kB detailed. Already in `images.md`; it is the reference point. | [Qt for MCUs: managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| GPU block formats have a fixed rate: BC1 4 bpp, BC7 and ASTC 4x4 8 bpp, ETC2 4 or 8 bpp, ASTC 12x12 0.89 bpp. ASTC 8x8 (2 bpp) shows artefacts. | [Texture compression in 2020](https://aras-p.info/blog/2020/12/08/Texture-Compression-in-2020/) | `summary` |

### 1.2 Features, limits and licence

| Finding | Link | Mark |
|---|---|---|
| QOI: 3 or 4 channels, 8 bit, alpha not premultiplied. All chunks are byte aligned. Decoder state is one previous pixel and a 64-entry colour array (256 bytes). | [qoi.h](https://github.com/phoboslab/qoi/blob/master/qoi.h) | `source` |
| QOI is MIT licensed. The format suits a streaming decoder "with minimal RAM"; the reference decoder itself loads the whole file. | [QOI README](https://github.com/phoboslab/qoi) | `source` |
| QOIR: QOI-like opcodes plus LZ4, in independent 64x64 tiles. Premultiplied alpha is possible. Apache 2.0. Marked experimental and subject to incompatible change. | [QOIR README](https://github.com/nigeltao/qoir), `QOIR_TILE_SIZE 0x40` in [qoir.h](https://github.com/nigeltao/qoir/blob/main/src/qoir.h) | `source` |
| WebP container: alpha for a lossy image is a separate `ALPH` chunk, raw 8 bit or VP8L-compressed. All channels are 8 bit. Animation has per-frame offset, blend and dispose. | [WebP container specification](https://developers.google.com/speed/webp/docs/riff_container) | `source` |
| A VP8L decoder needs: 14 predictor modes, colour transform, subtract-green, palette with pixel bundling, LZ77 with a 120-entry 2D distance map, a colour cache up to 2048 entries, five Huffman codes per group, and an entropy image. | [WebP lossless bitstream](https://developers.google.com/speed/webp/docs/webp_lossless_bitstream_specification) | `source` |
| VP8L back-references reach far back in scan order (copy length up to 4096). Lines cannot be decoded independently. | [WebP lossless bitstream](https://developers.google.com/speed/webp/docs/webp_lossless_bitstream_specification) | `source` |
| libwebp patent grant: "perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable", ended if the recipient sues over it. | [libwebp PATENTS](https://github.com/webmproject/libwebp/blob/main/PATENTS) | `source` |
| AV1 (AVIF): the AOM Patent License 1.0 uses the same words, with the same defensive termination. | [AOM Patent License 1.0](https://aomedia.org/license/patent-license/) | `source` |
| JPEG XL: reference software is "free and open source, royalty-free". Lossless, alpha, high bit depth, progressive and animation are in the format. | [jpeg.org: JPEG XL](https://jpeg.org/jpegxl/) | `source` |
| HEIC is "not royalty free": it is HEVC inside. JPEG and WebP are limited to 8 bit; JPEG has no alpha. | [Cloudinary: next-gen codecs](https://cloudinary.com/blog/time_for_next_gen_codecs_to_dethrone_jpeg) | `source` |
| HEVC has three patent pools, and not every patent holder is in one. | [Search summary of licensing articles](https://cloudinary.com/blog/how_jpeg_xl_compares_to_other_image_codecs) | `summary` |
| JPEG baseline as TJpgDec reads it: YCbCr or grey, sampling 4:4:4, 4:2:2 or 4:2:0, up to 65520 pixels a side. Progressive and lossless JPEG are refused. | [TJpgDec application note](http://elm-chan.org/fsw/tjpgd/en/appnote.html) | `source` |

### 1.3 Decode speed and memory on a desktop CPU

| Finding | Link | Mark |
|---|---|---|
| Decode speed relative to QOIR: LZ4 without filters 2.29, LZ4 over PNG filters 1.30, QOI 0.70, fpng 0.54, wuffs PNG 0.51, WebP lossless 0.33, libpng 0.20, JPEG XL lossless 0.02 to 0.12. | [QOIR README](https://github.com/nigeltao/qoir) | `source` |
| QOI decodes 226 megapixels per second, libpng 67, on all categories together. On 64-pixel icons: 274 against 46. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| libwebp decode peak memory: 76 kB plus 15 bytes per pixel for lossy, 133 kB plus 15 bytes per pixel for lossless. A 256x256 image peaks at 1.06 MB. | [webpx: WebP memory use](https://docs.rs/crate/webpx/0.1.4/source/docs/webp-memory-use.md) | `source` |
| That page measured libwebp 1.5 with heaptrack on x86-64. The crate version that carries it is marked yanked. | [webpx: WebP memory use](https://docs.rs/crate/webpx/0.1.4/source/docs/webp-memory-use.md) | `source` |

## 2. Decoders that already exist for microcontrollers

| Finding | Link | Mark |
|---|---|---|
| TJpgDec: 3.1 kB of code on Cortex-M0, 3100 bytes of work memory, output RGB888, RGB565 or grey, block by block. BSD-style licence. | [TJpgDec application note](http://elm-chan.org/fsw/tjpgd/en/appnote.html) | `source` |
| TJpgDec fast-decode level 2 adds a 6144-byte table. The note prints no decode times as text. | [TJpgDec application note](http://elm-chan.org/fsw/tjpgd/en/appnote.html) | `source` |
| JPEGDEC: needs 20 kB of RAM, draws one block of pixels per callback, scales by 1/2, 1/4, 1/8. Progressive files give only a DC thumbnail. Apache 2.0. | [JPEGDEC README](https://github.com/bitbank2/JPEGDEC) | `source` |
| JPEGDEC decodes a 640x480 JPEG in 1.3 s on a 48 MHz Cortex-M0+, and in 29.6 ms on a 600 MHz Cortex-M7. | [CNX Software on JPEGDEC](https://www.cnx-software.com/2020/08/31/jpegdec-faster-jpeg-arduino-library-designed-for-32-bit-mcus/) | `source` |
| That is about 200 CPU cycles per pixel on the M0+ and about 58 on the M7 (derived: time x clock / 307,200 pixels). | [CNX Software on JPEGDEC](https://www.cnx-software.com/2020/08/31/jpegdec-faster-jpeg-arduino-library-designed-for-32-bit-mcus/) | `source` |
| Espressif's newer JPEG decoder outputs RGB565 in strips: 16 rows for 4:2:0, 8 rows otherwise. TJpgDec sits in the ROM of ESP32, S3, C3 and C6. | [ESP new JPEG introduction](https://developer.espressif.com/blog/2025/09/esp-new-jpeg-introduction/) | `source` |
| PNGdec: needs 48 kB of free RAM, calls back per line, has an RGB565 conversion helper, no interlacing, no malloc. Apache 2.0. | [PNGdec README](https://github.com/bitbank2/PNGdec) | `source` |
| pngle: feed bytes, get a callback per pixel as RGBA. RAM is one scanline plus about 43 kB for inflate. Adam7 is supported. Needs miniz. MIT. | [pngle README](https://github.com/kikuchan/pngle) | `source` |
| The 32 kB DEFLATE window is the floor for any PNG decoder's RAM. Both small PNG decoders land above 40 kB because of it. | [pngle README](https://github.com/kikuchan/pngle) | `source` |
| AnimatedGIF: needs 24 kB of RAM, calls back per line. A "cooked" full canvas costs width x height x 3 bytes; turbo mode adds 32 kB. Apache 2.0. | [AnimatedGIF README](https://github.com/bitbank2/AnimatedGIF) | `source` |
| No WebP decoder by the same author turned up. No published flash or RAM number for libwebp on a microcontroller was found. | search of 2026-11-16 | `summary` |
| An ESP32-C3 user ran out of memory loading a 200 kB animated WebP. libwebp has no incremental API for animations. | [webp-discuss thread](https://groups.google.com/a/webmproject.org/g/webp-discuss/c/Z8-tdlweIrs) | `summary` |
| Alpine's decoder-only `libwebpdecoder` package installs at 314 kB on x86-64. It is a desktop build, not a stripped MCU figure. | [Alpine package](https://pkgs.alpinelinux.org/package/v3.24/main/x86_64/libwebpdecoder) | `summary` |
| LVGL's libwebp binding sizes the output as width x height x 4 bytes. It decodes the whole frame. | [LVGL WebP decoder](https://docs.lvgl.io/9.5/libs/image_support/libwebp.html) | `summary` |
| Wuffs ships GIF and PNG decoders that cannot allocate and do no I/O. PNG is 1.2 to 2.7 times faster than libpng. MIT or Apache 2.0. | [Wuffs README](https://github.com/google/wuffs) | `source` |
| J40: a single-file C99 JPEG XL decoder, public domain, "highly experimental". No progressive files, no animation, restoration filters ignored. Last version dated 2022-09. | [J40 README](https://github.com/lifthrasiir/j40) | `source` |
| tinyqoi: a Rust QOI decoder for `embedded-graphics`. It reads `include_bytes!` data and decodes at run time. | [tinyqoi](https://docs.rs/tinyqoi) | `summary` |
| PIF, a hobby format for MCUs: RGB888, RGB565, RGB332, indexed and 1 bit, optional simple RLE, 60 bytes of RAM, per-pixel callback. LGPL-2.1. No alpha is listed. | [PIF image format](https://github.com/gfcwfzkm/PIF-Image-Format) | `source` |
| PIF's RLE on a noisy photo saves little: RGB565 524,316 to 434,320 bytes. RLE needs flat art. | [PIF image format](https://github.com/gfcwfzkm/PIF-Image-Format) | `source` |

### Source size of the decoders (lines, whole file)

| Finding | Link | Mark |
|---|---|---|
| `lv_rle.c` 111. `qoi.h` 649, encoder, decoder and the format description together. | [lv_rle.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/libs/rle/lv_rle.c), [qoi.h](https://github.com/phoboslab/qoi/blob/master/qoi.h) | `measured` |
| PNG: `miniz_tinfl.c` 778 (inflate only), `pngle.c` 936 (without inflate), PNGdec `png.inl` 1055, `upng.c` 1281, `lodepng.cpp` 7273 (with encoder). | [miniz](https://github.com/richgel999/miniz), [pngle](https://github.com/kikuchan/pngle), [PNGdec](https://github.com/bitbank2/PNGdec), [upng](https://github.com/elanthis/upng), [lodepng](https://github.com/lvandeve/lodepng) | `measured` |
| JPEG: `tjpgd.c` 1139, `picojpeg.c` 2325, JPEGDEC `jpeg.inl` 5433. | [tjpgd.c in LVGL](https://github.com/lvgl/lvgl/blob/v9.6.0/src/libs/tjpgd/tjpgd.c), [picojpeg](https://github.com/richgel999/picojpeg), [JPEGDEC](https://github.com/bitbank2/JPEGDEC) | `measured` |
| GIF: AnimatedGIF `gif.inl` 1736. LZ4: `lz4.c` 2974, compressor included. | [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF), [lz4.c](https://github.com/lz4/lz4/blob/dev/lib/lz4.c) | `measured` |
| WebP: libwebp `vp8l_dec.c` 2189 and `vp8_dec.c` 759, each without Huffman, bit reader and DSP files. `simplewebp.h`, lossy and lossless in one file, 5656. | [libwebp src/dec](https://github.com/webmproject/libwebp/tree/main/src/dec), [simplewebp](https://github.com/MikuAuahDark/simplewebp) | `measured` |
| QOIR `qoir.h` 6994, of which its README says about 3000 are code and 4000 are tables. | [qoir.h](https://github.com/nigeltao/qoir/blob/main/src/qoir.h) | `measured` |
| JPEG XL: `j40.h` 8455, and it covers only a subset. `stb_image.h`, all its formats, 7798. | [j40.h](https://github.com/lifthrasiir/j40/blob/main/j40.h), [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) | `measured` |

## 3. Formats made for embedded UIs or for decode speed

| Finding | Link | Mark |
|---|---|---|
| TouchGFX stores RGB565 images with "a variation of QOI that is tailored for 16-bit pixel values" (QOI565). The variant is not published as a specification. | [TouchGFX image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX decompresses "in chunks by software" while drawing. The page names no offset table and does not say how a clipped image is handled. | [TouchGFX image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| ST builds a hardware decoder (Chrom-DEC) for these formats. Decoded pixels go to the framebuffer by DMA. | [TouchGFX image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| Qt for MCUs RLE formats (RGB888RLE, XRGB8888RLE, ARGB8888RLE) "can be decoded on the fly as they are being blended" and skip the cache. | [Qt for MCUs: managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt's cost for that: worse ratio than PNG, no transforms, CPU fallback on most platforms, slow when many pixels are semi-transparent. | [Qt for MCUs: managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt lists "the PNG and RLE decoders when the image data is invalid" as an attack surface for assets on external storage. | [Qt for MCUs: managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Espressif's split formats (SPNG, SJPG, SQOI) cut the image into 16-line strips, each a complete small file. Already in `images.md`; it is strip-level random access. | [ESP LV decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html) | `source` |
| Espressif reports about 40 times less RAM for split decode than for full decode. | [esp_lvgl_adapter example](https://components.espressif.com/components/espressif/esp_lvgl_adapter/versions/0.6.0-beta/examples/lvgl_decode_image) | `summary` |
| QOIR's tiles are independent 64x64 blocks. That is tile-level random access with LZ4 inside, on a desktop format. | [QOIR README](https://github.com/nigeltao/qoir) | `source` |
| Allegro's RLE sprites replace runs of transparent pixels by a count, and prefix solid runs with a length. Drawing skips a transparent run "with a single add". | [Allegro manual: RLE sprites](https://liballeg.org/stabledocs/en/alleg015.html) | `source` |
| The same price as everywhere: "you can't flip them, rotate them, or stretch them". | [Allegro manual: RLE sprites](https://liballeg.org/stabledocs/en/alleg015.html) | `source` |
| heatshrink, an LZSS for MCUs: the decoder is about 1 kB of code on AVR and can run in under 50 bytes of RAM. | [heatshrink article](https://spin.atomicobject.com/heatshrink-embedded-data-compression/) | `summary` |
| LVGL's RLE, LZ4 and every bundled file decoder except BMP, tjpgd and indexed need the whole frame in RAM. From `images.md`; no LVGL form streams with compression and alpha. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |

### Which can be decoded while blending, without a frame buffer

| Format | RAM beyond the output line | Start at any line | Mark |
|---|---|---|---|
| Per-row RLE (Qt for MCUs, Allegro style) | None | Yes, if row starts are stored or rows are walked | `source` (Qt, Allegro pages above) |
| QOI, QOI565 | 256 bytes of state | No: state runs from the first pixel. Yes per strip if the state is reset (SQOI) | `source` (qoi.h, ESP page) |
| Baseline JPEG | About 3.1 kB | Per 8- or 16-row block band, in order. Restart markers could give entry points | `source` (TJpgDec note); restart markers `recalled` |
| LZ4 or LZ77 on pixels | The window: the output so far | Only per independent tile or strip (QOIR, split formats) | `source` (QOIR README) |
| PNG | 32 kB window plus two lines | No, unless cut into strips (SPNG) | `source` (pngle README, ESP page) |
| WebP lossless, WebP lossy, AVIF, JPEG XL | Whole frame in practice | No | `source` (VP8L specification, webpx memory page) |

## 4. What an MCU UI format needs that web formats do not target

| Finding | Link | Mark |
|---|---|---|
| Web formats optimise bytes on the wire. Their decoders assume megabytes of RAM: libwebp peaks at 15 bytes per pixel. | [webpx: WebP memory use](https://docs.rs/crate/webpx/0.1.4/source/docs/webp-memory-use.md) | `source` |
| Decode to RGB565 directly: TJpgDec, Espressif's JPEG decoder and QOI565 do it. PNG, QOI and WebP produce 8-bit channels that must be converted. | [TJpgDec application note](http://elm-chan.org/fsw/tjpgd/en/appnote.html) | `source` |
| No RAM beyond a line: per-row RLE and QOI meet it. Anything with an LZ window does not, unless the window is the strip itself. | rows of section 3 | `source` |
| Line-level entry: only strip formats (Espressif, 16 lines) and tile formats (QOIR, 64x64) provide it. Both restart the codec per unit. | [ESP LV decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html) | `source` |
| Transparent-run skipping makes a sparse sprite faster to draw than its raw form, not slower. | [Allegro manual: RLE sprites](https://liballeg.org/stabledocs/en/alleg015.html) | `source` |
| LVGL already has the hook: a decoder may serve one line at a time (`get_area`). Such images cannot be rotated or scaled. From `images.md`. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| No published format was found that combines RGB565 with separate alpha, a per-line offset table and per-line RLE. Vendors do this privately. | two searches of 2026-11-16 | `summary` |
| The nearest open designs are PIF (no alpha, no line table, LGPL) and Espressif's split containers (generic codecs in strips). | [PIF](https://github.com/gfcwfzkm/PIF-Image-Format), [ESP LV decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html) | `source` |
| Dithering and RLE work against each other (MicroEJ, in `images.md`). A photo dithered to RGB565 must not go through a run-length form. | [MicroEJ: colour display considerations](https://docs.microej.com/en/latest/Trainings/trainingColorDisplayConsiderations.html) | `source` |

## 5. Decoding in constant evaluation

| Finding | Link | Mark |
|---|---|---|
| Our PNG decoder under GCC 16.2: 48x48 RGBA about 1 s; 320x240 RGBA 25 to 37 s and 2 to 4 GB. Half is inflate, half unfiltering and conversion. | our spike, 2026-11 | `measured` |
| That is roughly 0.4 ms per pixel in the interpreter (derived: 30 s / 76,800 pixels). Operations per pixel are the budget. | our spike, 2026-11 | `measured` |
| `const_qoi` (Rust, ISC): a QOI decoder and encoder where "every public and private function is const including all the tests". | [const_qoi](https://docs.rs/const_qoi) | `source` |
| It is the only compile-time-capable image decoder found. Its page shows no full image decoded in a `const` item. | [const_qoi](https://docs.rs/const_qoi) | `source` |
| Zig: `zig-pcx` is reported to decode an `@embedFile` PCX inside a `comptime` block with a raised branch quota. PCX is byte RLE. | [zig-pcx](https://github.com/dbandstra/zig-pcx) | `summary` |
| No compile-time decoder for PNG, JPEG, WebP, AVIF or JPEG XL was found in C++, Rust or Zig, in five searches. | searches of 2026-11-16 | `summary` |
| QOI has no entropy coder and no window: one byte-aligned opcode per pixel or run. It removes the inflate half of our PNG cost by construction. | [qoi.h](https://github.com/phoboslab/qoi/blob/master/qoi.h) | `source` |
| QOI's site says its encoder and decoder fit in about 300 lines of C; the whole file is 649. The decoder alone was not counted. | [qoiformat.org](https://qoiformat.org) | `summary` |
| A baseline JPEG decoder is about 1100 lines (tjpgd). | line counts in section 2 | `measured` |
| A VP8L decoder is over 2200 lines before Huffman and bit-reader code, and has eight coding tools. JPEG XL needs over 8000 lines for a subset. | line counts in section 2 | `measured` |

## 6. What design tools export

| Finding | Link | Mark |
|---|---|---|
| Figma exports PNG, JPG, SVG and PDF. Its help page lists no WebP, AVIF or GIF. | [Figma: export formats and settings](https://help.figma.com/hc/en-us/articles/13402894554519-Export-formats-and-settings) | `source` |
| Figma exports PNG and JPG at scale factors (`2x`, fixed width, fixed height). SVG and PDF export at 1x only. | [Figma: export formats and settings](https://help.figma.com/hc/en-us/articles/13402894554519-Export-formats-and-settings) | `source` |
| Penpot exports PNG, JPEG, WebP, SVG and PDF. WebP was added in March 2025. | [Penpot: exporting layers](https://help.penpot.app/user-guide/exporting) | `summary` |
| Lottie in LVGL renders through ThorVG into a caller's buffer. It is the existing route for vector animation. From `images.md`. | [lottie.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/widgets/lottie.mdx) | `source` |

## Not confirmed

- Flash size of PNGdec, JPEGDEC and AnimatedGIF. Their READMEs give RAM only. Benchmarks are charts the fetch tool could not read.
- Flash and RAM of libwebp built for a Cortex-M. No number was found. The 314 kB figure is a desktop shared library.
- CPU cycles per pixel for PNG, QOI or LZ4 on a Cortex-M. Only JPEG has a published number (JPEGDEC, above).
- Code size and RAM of a streaming QOI decoder on an MCU. Expected a few hundred bytes of code and 256 bytes of state. `recalled`
- VP8 (lossy WebP) stores colour as 4:2:0 only, so thin coloured lines smear. The container page does not say it. `recalled`
- AVIF is about half the size of JPEG at equal quality. dav1d is far above 100 kB of code and needs megabytes of RAM. `recalled`
- No small AVIF decoder exists. A JPEG XL decoder in Rust (`jxl-rs`, `jxl-oxide`) exists; its size on an MCU is unknown. `recalled`
- NanoJPEG is about 900 lines but decodes the whole frame. picojpeg needs about 2.3 kB of RAM. stb_image decodes whole frames with `malloc`. `recalled`
- GIF's LZW patents expired in 2003 and 2004. GIF is limited to 256 colours per frame and 1-bit transparency. `recalled`
- JPEG restart markers as entry points for random access into a baseline JPEG. Possible by the standard; no MCU decoder was seen using it. `recalled`
- JPEG XS: a low-latency, line-based wavelet codec for video links, with a patent pool. Not aimed at stored UI art. `recalled`
- BlurHash and ThumbHash: 20 to 30 byte placeholders for a blurred preview. Not an image format for us. `recalled`
- How GPU block formats look on UI text and icons. Godot's advice against them for 2D is in `images.md`; no measurement was found.
- QOIR's file-format document: the link returned 404. The tile size comes from the header source instead.
- Whether `zig-pcx` really decodes at comptime: the README could not be fetched; only a search summary was seen.
- Whether Figma can export WebP or GIF through plugins or newer versions. The help page read here lists four formats.
- QOI565's opcode layout. ST's page describes it in one sentence.
- emWin's RLE formats. Still unread, as in `images.md`.
- The constant-evaluation cost of QOI and of baseline JPEG under GCC 16. Nobody has published one; we must measure.

## What it means for us

1. Tools export PNG, JPG and SVG everywhere. WebP comes from Penpot only. So PNG and baseline JPEG are the source formats that matter.
2. No web format is a good stored form on a microcontroller. WebP, AVIF and JPEG XL need the whole frame and megabytes of working RAM.
3. Their gain over PNG is real but modest for UI art: WebP lossless is about two thirds of PNG. That saving does not survive a full-frame RAM buffer.
4. Baseline JPEG is the exception. It streams in 8- or 16-row bands, needs about 3 kB of RAM, and outputs RGB565. LVGL already ships the decoder.
5. For photos, JPEG's 10:1 beats every lossless form by a factor of five. Nothing we could write ourselves competes with it.
6. For UI art, the stored form must be decodable per line with no window. That leaves per-row RLE and QOI-style coding. Everything else needs a buffer.
7. QOI on icons is 1.2 to 1.7 times PNG's size, and still 3 to 12 times smaller than raw. It needs 256 bytes of RAM.
8. QOI cannot start at an arbitrary line. Resetting its state per strip (as SQOI does) fixes that at a small size cost.
9. Per-row RLE with alpha-aware runs (skip transparent, copy opaque, blend the rest) is what Qt and Allegro do. It can draw faster than raw for sparse art.
10. LVGL's own RLE and LZ4 do not have these properties: they need the whole frame on the heap. That gap is the case for an own stored form.
11. An own format is justified only as a thin container, not as a new codec. It holds a header, a row or strip offset table, and RGB565 and A8 coded per row.
12. Such a form plugs into LVGL as a custom decoder that serves lines. It inherits the known limit: no rotation and no scaling.
13. For the compile-time path, decoder cost per pixel decides. QOI has no inflate, so it should cost about half of PNG per pixel. Measure before promising.
14. QOI as a source format helps large images compile, but no design tool exports it. It needs a converter step, so it is an option, not the default.
15. A consteval baseline JPEG decoder is about 1100 lines and feasible. VP8L is twice that with eight coding tools. AVIF and JPEG XL are out of reach.
16. Animations: GIF frames need a full canvas (width x height x 3). Animated WebP has no incremental decoder. Lottie through ThorVG stays the vector route.
17. GPU block formats, JPEG XS, HEIC and placeholder hashes solve other problems. None should be accepted or stored.

### Recommendation

Source formats to accept (decoded by the compiler):

1. PNG, as today. It is what every tool exports for art with alpha. Reduce its cost per pixel before adding formats.
2. Baseline JPEG, second. Only needed when a photo should be stored as raw RGB565; otherwise the bytes pass through untouched (see below).
3. QOI, third and optional, as the cheap-to-compile source for big images. Decide after measuring it against PNG at 320x240.
4. WebP lossless only if Penpot users ask for it. Not AVIF, JPEG XL or HEIC.

Stored forms to implement when pixels are not raw:

1. Photos and backgrounds: the JPEG bytes as they are, decoded on the device by tjpgd in bands. No new code beyond wiring and a compile-time check that the file is baseline.
2. Icons and UI art with alpha: first measure LVGL's RLE on our own icon set, to know what a size mode is worth at all.
3. Then, if the saving is worth it: an own row-addressable form. RGB565 plus A8, coded per row with alpha-aware runs, with a row or strip offset table, served through an LVGL line decoder.
4. Compare that form against QOI565-style coding in strips on the same icon set, for size and for cycles per pixel on the Cortex-M4 in QEMU. Keep one.
5. Animations: a sequence of frames in the same stored form for small sprites; Lottie for vector motion. No GIF or WebP decoder of our own.

Is an own format justified? Yes, narrowly: as a per-row container that LVGL lacks and every commercial framework has. No, as a new compression scheme: per-row RLE and QOI already cover the design space that needs no RAM.
