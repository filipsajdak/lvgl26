# Image storage formats for the compile-time path

Checked 2026-11-16. Question: which stored forms should the compile-time image path produce, and how should it choose.

## How the marks were earned

- `source` on an LVGL row: the file was read in the local checkout (`build/_src/lvgl-v9.6.0`, and the v10 pin in `build/host-v10/_deps/lvgl-src`).
- The LVGL links point at the same path upstream under tag `v9.6.0`. The upstream URL itself was not opened.
- `source` on a web row: the page was fetched and read through a fetch tool that extracts text. Quotes are as the tool returned them.
- `summary`: only a search engine's summary was seen.
- `recalled`: from memory, no source found. All of these are under "Not confirmed".

## 1. What LVGL draws straight from read-only memory

| Finding | Link | Mark |
|---|---|---|
| `lv_image_dsc_t` is a 12-byte header, `data_size`, `data` and two reserved pointers. It has the same size as `lv_draw_buf_t`. | [lv_image_dsc.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/draw/lv_image_dsc.h) | `source` |
| Header fields in order: `magic` (8 bit, `0x19`), `cf` (8), `flags` (16), `w` (16), `h` (16), `stride` (16), `reserved_2` (16). The order is reversed on big-endian systems. | [lv_image_dsc.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/draw/lv_image_dsc.h) | `source` |
| Flags that matter for a constant image: `PREMULTIPLIED` (0x0001) and `COMPRESSED` (0x0008). `ALLOCATED`, `MODIFIABLE` and `CUSTOM_DRAW` are for draw buffers. Bits 8 to 15 are for the user. | [lv_image_dsc.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/draw/lv_image_dsc.h) | `source` |
| An uncompressed C-array image in RGB565, RGB565 swapped, RGB565A8, RGB888, XRGB8888, ARGB8888, ARGB8888 premultiplied, L8, AL88 or A8 is used in place. The decoder only wraps the pointer in a draw buffer. | [lv_bin_decoder.c, `lv_bin_decoder_open`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| "No heap" is not exact. Every open allocates one small zeroed `decoder_data_t` with `lv_malloc` and frees it on close. No pixel data is copied. | [lv_bin_decoder.c, `get_decoder_data`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| A directly usable image is never put in the image cache. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| Indexed images (I1, I2, I4, I8) store a palette of 2, 4, 16 or 256 `lv_color32_t` entries in front of the pixel indices. | [lv_bin_decoder.c, `decode_indexed`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| The software renderer cannot draw an indexed image. With `LV_BIN_DECODER_RAM_LOAD` the decoder converts the whole image to ARGB8888 on the heap (`w * h * 4` bytes). | [lv_bin_decoder.c, `decode_indexed`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| Without `LV_BIN_DECODER_RAM_LOAD` an indexed image is converted one line at a time into a one-line ARGB8888 buffer. | [lv_bin_decoder.c, `lv_bin_decoder_get_area`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| A line-by-line image cannot be rotated or scaled. LVGL logs "Partially decoded images cannot be transformed". | [lv_draw_image.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/lv_draw_image.c) | `source` |
| Only the VG-Lite and NemaGFX draw units ask for indexed data as it is (`use_indexed = true`). | [lv_vg_lite_utils.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/vg_lite/lv_vg_lite_utils.c) | `source` |
| The docs say indexed formats need `LV_DRAW_SW_SUPPORT_ARGB8888`, and palette colours are always ARGB8888. | [color_formats.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/main-modules/images/color_formats.mdx) | `source` |
| A1, A2 and A4 are always expanded to a full A8 buffer on the heap (`w * h` bytes) at open. A8 is used in place. | [lv_bin_decoder.c, `decode_alpha_only`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| ARGB8565, ARGB1555, ARGB4444, ARGB2222 and A1/A2/A4 are marked as not supported by the software renderer. | [lv_color.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/draw/lv_color.h) | `source` |
| An A8 image is drawn as a mask filled with the draw descriptor's recolor colour. This is the cheapest path for a tinted icon. | [lv_draw_sw_img.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/lv_draw_sw_img.c) | `source` |
| RGB565A8 is a colour plane followed by an A8 plane. The alpha stride is half the colour stride. Untransformed and not recoloured, it is drawn as RGB565 with a mask. | [lv_draw_sw_img.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/lv_draw_sw_img.c) | `source` |
| RGB565 drawn opaque onto an RGB565 target is one `lv_memcpy` per line. Any other source format is converted pixel by pixel. | [lv_draw_sw_blend_to_rgb565.c, `rgb565_image_blend`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c) | `source` |
| The RGB565 target accepts as blend sources: RGB565, RGB565 swapped, RGB888, XRGB8888, ARGB8888, ARGB8888 premultiplied, L8, AL88, I1. | [lv_draw_sw_blend_to_rgb565.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c) | `source` |
| Each source format can be compiled out with `LV_DRAW_SW_SUPPORT_*`. An image in a disabled format will not draw. | [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/v9.6.0/lv_conf_template.h) | `source` |
| Default stride is `ceil(w * bpp / 8)` rounded up to `LV_DRAW_BUF_STRIDE_ALIGN` (default 1). A header stride of 0 means the v8 layout. | [lv_draw_buf.c, `width_to_stride`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/lv_draw_buf.c) | `source` |
| If `LV_DRAW_BUF_STRIDE_ALIGN` is not 1 and the image stride differs, the decoder copies the image to the heap. RGB565A8 is exempt. | [lv_image_decoder.c, `lv_image_decoder_post_process`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_image_decoder.c) | `source` |
| If the draw unit asks for premultiplied alpha and the image is not premultiplied, the decoder duplicates it on the heap. VG-Lite and the software vector renderer ask for it. | [lv_image_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_image_decoder.c) | `source` |
| `LV_DRAW_BUF_ALIGN` (default 4) is the expected address alignment. A misaligned constant image only produces an info log. | [lv_draw_buf.c, `lv_draw_buf_init`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/lv_draw_buf.c) | `source` |
| `data_size` must be at least `stride * h`, or the image is rejected. | [lv_draw_buf.c, `lv_draw_buf_init`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/lv_draw_buf.c) | `source` |
| `LV_COLOR_DEPTH` picks the default display format. Its alpha companion is RGB565A8 at depth 16 and ARGB8888 at depth 32. | [color_formats.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/main-modules/images/color_formats.mdx) | `source` |
| The DMA2D draw unit names RGB565, RGB888, XRGB8888, ARGB8888 and ARGB1555. No indexed or A8 image path was seen there. | [src/draw/dma2d](https://github.com/lvgl/lvgl/tree/v9.6.0/src/draw/dma2d) | `source` |

## 2. LVGL's own compression

| Finding | Link | Mark |
|---|---|---|
| A compressed image sets `LV_IMAGE_FLAGS_COMPRESSED`. Its data starts with 12 bytes: method (low 4 bits of a `uint32`), compressed size, decompressed size. | [lv_bin_decoder.c, `lv_image_compressed_t`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| Methods: 0 none, 1 RLE, 2 LZ4. `data_size - 12` must equal the compressed size, or the open fails. | [lv_image_dsc.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/draw/lv_image_dsc.h) | `source` |
| A compressed image needs `LV_BIN_DECODER_RAM_LOAD`. Without it the open fails with an error. | [lv_bin_decoder.c, `decode_compressed`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| The whole image is decompressed into one heap draw buffer (`stride * h`). There is no line-by-line decompression. The header comment says the same. | [lv_bin_decoder.c, `decompress_image`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| A compressed indexed image has its palette inside the compressed payload. Under the software renderer it is then converted to ARGB8888 as well: two heap buffers. | [lv_bin_decoder.c, `decode_indexed`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| RLE works on pixel-sized blocks. A control byte with bit 7 set copies up to 127 literal blocks; otherwise it repeats one block up to 127 times. | [lv_rle.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/libs/rle/lv_rle.c) | `source` |
| The reference encoder stores a run as a repeat only when it is 16 blocks or longer. | [LVGLImage.py, `rle_compress`](https://github.com/lvgl/lvgl/blob/v9.6.0/scripts/LVGLImage.py) | `source` |
| The RLE decoder is about 110 lines of C. LZ4 uses the bundled `lz4.c` (`LV_USE_LZ4_INTERNAL`) or an external library, through `LZ4_decompress_safe`. | [lv_rle.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/libs/rle/lv_rle.c) | `source` |
| LVGL's docs claim RLE saves more than 70% on most UI images of a watch project. No decode times are given. | [rle.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/libs/image_support/rle.mdx) | `source` |
| A decompressed image goes into the image cache. The cache is off by default: `LV_CACHE_DEF_SIZE` is 0. Then every draw decompresses again. | [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/v9.6.0/lv_conf_template.h) | `source` |
| The cache is sized in bytes and evicts least recently used entries. The user must leave room for the largest images. | [caching.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/main-modules/images/caching.mdx) | `source` |
| v9.6.0 and the v10 pin have identical `lv_bin_decoder.c`, `lv_image_decoder.c`, `lv_rle.c`, `lv_image_cache.c`, `lv_image_dsc.h`, `lv_lodepng.c`, `lv_tjpgd.c` and `lv_bmp.c`. | local `diff` of the two trees | `source` |
| The v10 pin removes the deprecated `lv_image_buf_set_palette` and `lv_image_buf_free`. Its software image draw takes a layer and clip area instead of a task. | local `diff` of `lv_draw_buf.c`, `lv_draw_sw_img.c` | `source` |

## 3. Other decoders LVGL ships

| Finding | Link | Mark |
|---|---|---|
| lodepng decodes the whole PNG to ARGB8888. It needs `w * h * 4` bytes from the LVGL heap. The docs advise the image cache because decoding is slow. | [lodepng.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/libs/image_support/lodepng.mdx) | `source` |
| libpng and libjpeg-turbo also decode the whole image to RAM and "need" the image cache. They are external libraries. | [libpng.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/libs/image_support/libpng.mdx) | `source` |
| tjpgd decodes baseline JPEG in 8x8 tiles to RGB888 with a 4096-byte work buffer. Such images cannot be zoomed or rotated. | [lv_tjpgd.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_tjpgd.c) | `source` |
| TJpgDec itself: 3.1 kB of code on Cortex-M0, a work area up to 3100 bytes, plus 320 bytes at fast-decode level 1. | [TJpgDec application note](http://elm-chan.org/fsw/tjpgd/en/appnote.html) | `source` |
| BMP is read line by line and needs very little RAM. It works from files only. | [bmp.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/libs/image_support/bmp.mdx) | `source` |
| GIF needs about 25 kB plus `(pixel_size + 1) * w * h` bytes. | [gif.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/widgets/gif.mdx) | `source` |
| Lottie renders through ThorVG (C++) into a buffer the user provides. | [lottie.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/widgets/lottie.mdx) | `source` |
| LVGL has no QOI decoder. A search of `src` and `docs` for "qoi" finds nothing. | local `grep` of the v9.6.0 tree | `source` |
| A WebP decoder binding exists (`lv_libwebp.c`), files only. | [adding_images.mdx](https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/main-modules/images/adding_images.mdx) | `source` |
| PNGdec, a PNG decoder for MCUs, asks for at least 48 kB of free RAM. Its time splits about evenly between inflate and unfiltering. | [PNGdec README](https://github.com/bitbank2/PNGdec) | `source` |

## 4. What other frameworks choose at build time

| Finding | Link | Mark |
|---|---|---|
| Slint: an image whose visible pixels share one colour (channels within 2) becomes an 8-bit alpha map plus that colour. | [embed_images.rs, `generate_texture`](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Slint: otherwise a fully opaque image becomes RGB, and anything else becomes premultiplied RGBA. Plain RGBA is never chosen. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Slint: the transparent border is cropped first. A pixel with alpha 0 or 1 counts as transparent. The texture keeps the original size and the crop rectangle. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Slint: a fully transparent image becomes a 1x1 texture. Raster images are only scaled down at build time, never up. SVG is rasterised at the scale factor. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Slint: this mode stores uncompressed pixels and only the software renderer can use them. The other mode embeds the file and decodes at run time. | [EmbedResourcesKind](https://docs.slint.dev/latest/docs/rust/slint_build/enum.EmbedResourcesKind) | `summary` |
| Qt for MCUs: the pixel format defaults to `Automatic`, chosen from "platform settings and image properties". | [resourceImagePixelFormat](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourceimagepixelformat.html) | `source` |
| Qt for MCUs: the platform sets one default format for opaque images and one for images with alpha. The opaque default is XRGB8888. | [platformOpaquePixelFormat](https://doc.qt.io/QtForMCUs/qtul-qmlproject-mcu-confignode-platformopaquepixelformat.html) | `summary` |
| Qt for MCUs: a single-colour image with alpha is detected and stored as an alpha map. An image with a big transparent border is cropped. | [Image optimizations](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html) | `source` |
| Qt for MCUs: the resource compiler can also lower colour depth and split an image into opaque segments. Splitting can cause artefacts on transformed images. | [Managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt for MCUs: RLE formats are decoded during blending and skip the cache. They compress worse than PNG, cannot be transformed, and blend slower with alpha. | [Managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt for MCUs example sizes, uncompressed / RLE / PNG: 82.9 / 23.2 / 6.7 kB for a flat image, 82.9 / 72.6 / 27.5 kB for a detailed one. | [Managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt for MCUs: `resourceCompression` stores PNG, links LodePNG, and forces the `OnDemand` cache policy. It is off by default. | [resourceCompression](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourcecompression.html) | `source` |
| Qt for MCUs cache policies: `OnStartup` copies to RAM at start, `OnDemand` when needed, `NoCaching` draws from flash and may be slower. | [resourceCachePolicy](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourcecachepolicy.html) | `source` |
| Qt for MCUs guidance: plenty of memory, cache at startup. Hardware RLE, use RLE. Limited RAM, PNG. Very limited RAM, lossless RLE or uncompressed without cache. | [Managing resources](https://doc.qt.io/QtForMCUs/qtul-resources.html) | `source` |
| Qt for MCUs: rotation and scale optimisations are opt-in per image, because another stored format may then be better. | [Image optimizations](https://doc.qt.io/QtForMCUs/qtul-qtquick-non-api-differences.html) | `source` |
| TouchGFX: L8 is a palette of up to 256 colours and one byte per pixel. A 200x200 logo falls from 120,000 to 40,048 bytes. | [Using the L8 image format](https://support.touchgfx.com/docs/development/ui-development/scenarios/using-the-l8-image-format-to-reduce-memory-consumption) | `source` |
| TouchGFX: the converter refuses L8 when the image has more than 256 colours. It does not quantise for you. | [Using the L8 image format](https://support.touchgfx.com/docs/development/ui-development/scenarios/using-the-l8-image-format-to-reduce-memory-consumption) | `source` |
| TouchGFX: the L8 palette format follows the hardware. Opaque uses RGB888, transparent ARGB8888; an RGB565 palette only where there is no DMA2D. | [Using the L8 image format](https://support.touchgfx.com/docs/development/ui-development/scenarios/using-the-l8-image-format-to-reduce-memory-consumption) | `source` |
| TouchGFX L8 compression: L4 for up to 16 colours, RLE for up to 64 colours, LZW9 for any L8 image. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX "Auto" takes the best ratio, but prefers RLE when LZW9 is only marginally smaller, because RLE decodes much faster. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX does not compress when the result is not below 90% of the original. The flash saved does not pay for slower drawing. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX compresses RGB888 and ARGB8888 with QOI and RGB565 with a QOI565 variant, since version 4.24. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX decode RAM: LZW9 needs a 2048-byte buffer. L4, RLE, QOI and QOI565 need none. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX: compressed images draw slower and bypass the 2D accelerator. Widgets that scale, rotate or texture-map cannot use them. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX: the fix for those widgets is to decompress into the RAM bitmap cache at run time, sized for the whole image. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX example ratios: an L8 184x184 image to 16.9% of its pixel data; an ARGB8888 240x240 image to about 14%. | [Image compression](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-compression) | `source` |
| TouchGFX offers per-image dithering (none, Floyd-Steinberg, Jarvis-Judice-Ninke, Stucki) and a switch to dither alpha too. | [Images view](https://support.touchgfx.com/docs/development/ui-development/designer-user-guide/images-view) | `source` |
| TouchGFX: opaque formats are "usually faster" because the background behind them is not drawn. | [Image formats](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/image-formats) | `source` |
| Embedded Wizard formats: Native (colour and opacity), Alpha8 (opacity only, colour given at run time), Index8 (palette), RGB565 (no opacity). | [Bitmap resource](https://doc.embedded-wizard.de/bitmap-resource-member) | `source` |
| Embedded Wizard compresses every bitmap by default. The alternative is the target's native format read straight from ROM. Dithering defaults to "Auto". | [Bitmap resource](https://doc.embedded-wizard.de/bitmap-resource-member) | `source` |
| MicroEJ offers a "display" output format equal to the display's own encoding, alpha-only A8/A4/A2/A1, and RLE formats. | [Image format](https://docs.microej.com/en/latest/VEEPortingGuide/uiImageRaw.html) | `source` |
| MicroEJ pads the pixel array to an aligned address, and adds row padding for a GPU. | [Image format](https://docs.microej.com/en/latest/VEEPortingGuide/uiImageRaw.html) | `source` |
| MicroEJ: dithering works against RLE. A dithered image "may end up larger than the same image in an uncompressed format". | [Colour display considerations](https://docs.microej.com/en/latest/Trainings/trainingColorDisplayConsiderations.html) | `source` |
| MicroEJ: RGB565 conversion truncates, so close-colour gradients band. Their example has 23 colours banded, 45 dithered, 170 in RGB888. | [Colour display considerations](https://docs.microej.com/en/latest/Trainings/trainingColorDisplayConsiderations.html) | `source` |
| Espressif: a standard PNG, JPEG or QOI needs RAM for the whole decoded image. | [ESP LV decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html) | `source` |
| Espressif's answer is split formats (SPNG, SJPG, SQOI): the image is cut into 16-line strips decoded one at a time. They cannot be zoomed or rotated. | [ESP LV decoder](https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/tools/esp_lv_decoder.html) | `source` |
| Espressif enables only the split formats on ESP32-C3 to limit RAM. Its advice: JPEG for opaque images, PNG or QOI for lossless alpha. | [lvgl_decode_image example](https://components.espressif.com/components/espressif/esp_lvgl_adapter/versions/0.7.5/examples/lvgl_decode_image) | `source` |
| QOI on a desktop CPU decodes 3.4 times faster than libpng (226 vs 67 megapixels per second) and is 16% larger. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| On 512-pixel icons QOI is 1.7 times the size of libpng output (8.4% vs 5.0% of raw). On 64-pixel icons the two are about equal. | [QOI benchmark](https://qoiformat.org/benchmark/) | `source` |
| On the same suite, LZ4 over PNG-filtered pixels is 1.40 times QOIR's size and decodes 1.3 times as fast. QOI is 1.12 times the size at 0.70 the speed. | [QOIR README](https://github.com/nigeltao/qoir) | `source` |
| On computer-generated images and screenshots PNG compresses best; QOI and JPEG-LS are comparable. | [CAST comparative study](https://cast-inc.com/blog/lossless-compression-efficiency-jpeg-ls-png-qoi-and-jpeg2000-comparative-study) | `summary` |

## 5. Build-time steps a compiler can do

| Finding | Link | Mark |
|---|---|---|
| Single colour plus alpha to A8: done by Slint, Qt for MCUs and Embedded Wizard. In LVGL it is 1 byte per pixel, no heap, and the fastest alpha path. It changes speed and size. | [lv_draw_sw_img.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/lv_draw_sw_img.c) | `source` |
| Opaque to the display's own format: on an RGB565 display it turns drawing into a line copy. It changes speed and size. | [lv_draw_sw_blend_to_rgb565.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c) | `source` |
| Colour with alpha on an RGB565 display to RGB565A8: 3 bytes per pixel instead of 4, and no 8888-to-565 conversion per pixel. Speed and size. | [lv_draw_sw_img.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sw/lv_draw_sw_img.c) | `source` |
| Cropping the transparent border: done by Slint and Qt. Fewer pixels are stored and fewer are blended. Speed and size. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Palette detection (I8, I4): halves flash or better, but under LVGL's software renderer it costs RAM or transformability, and draws slower. Size only. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| A1, A2, A4 masks: smaller in flash, but LVGL expands them to A8 on the heap at every open. Size only, and it costs RAM. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| Premultiplying at build time avoids a heap duplicate where the draw unit wants premultiplied input (VG-Lite, vector). It is no gain for the plain software renderer. | [lv_image_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_image_decoder.c) | `source` |
| Writing the stride LVGL expects for the configured `LV_DRAW_BUF_STRIDE_ALIGN` avoids a heap copy at open. Speed and RAM. | [lv_image_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_image_decoder.c) | `source` |
| Dithering to RGB565 removes banding on gradients. It makes RLE worse. Quality only; LVGL's own converter offers it as an option. | [LVGLImage.py](https://github.com/lvgl/lvgl/blob/v9.6.0/scripts/LVGLImage.py) | `source` |
| RLE or LZ4 at build time saves flash but always costs a full-image heap buffer in LVGL, and a cache to avoid repeating it. Size only; slower. | [lv_bin_decoder.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/image/lv_bin_decoder.c) | `source` |
| Scaling down at build time to the size used on screen is done by Slint. It avoids run-time scaling and stores fewer pixels. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |
| Slint's embed pass ignores nine-slice information. No framework read here splits a nine-patch at build time. | [embed_images.rs](https://github.com/slint-ui/slint/blob/master/internal/compiler/passes/embed_images.rs) | `source` |

## 6. Desktop and Linux targets

| Finding | Link | Mark |
|---|---|---|
| LVGL's SDL draw unit renders each draw task with the software renderer into an ARGB8888 buffer, uploads it as a texture, and caches up to 128 textures. | [lv_draw_sdl.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/sdl/lv_draw_sdl.c) | `source` |
| The OpenGL ES draw unit works the same way, with `LV_DRAW_OPENGLES_TEXTURE_CACHE_COUNT` (default 64) textures. | [lv_draw_opengles.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/draw/opengles/lv_draw_opengles.c) | `source` |
| The OpenGL driver uploads L8, RGB565, RGB888 and the 8888 formats with `glTexImage2D`. A shader flag handles premultiplied sources. | [lv_opengles_driver.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/drivers/opengles/lv_opengles_driver.c) | `source` |
| LVGL's own code never calls `glCompressedTexImage2D`. Compressed GPU textures have no path into the 2D renderer. | local `grep` of `src` (only glad, VG-Lite driver and FT81x headers match) | `source` |
| Godot's guidance for 2D: use lossless textures. GPU-compressed textures "should be avoided for 2D" because of visible artefacts at low resolution. | [Godot: importing images](https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_images.html) | `source` |
| So on desktop the stored format is paid once per cached texture. Raw ARGB8888 or XRGB8888 is the direct choice for a 32-bit display. | inference from the three rows above | `source` |

## 7. Cost of decoding PNG in constant evaluation

| Finding | Link | Mark |
|---|---|---|
| GCC stops a constant evaluation after 33,554,432 operations and a loop after 262,144 iterations. Both limits are flags. | [GCC C++ dialect options](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html) | `source` |
| No constexpr PNG decoder or constexpr inflate was found in three searches. Every decoder found runs at run time. | searches of 2026-11-16 | `summary` |
| Nearest prior art: constixel, a constexpr C++20 2D renderer. It only writes uncompressed PNG, and tells users to raise the ops limit. | [constixel](https://github.com/tinic/constixel) | `summary` |
| Nearest prior art for compression: consteval-huffman compresses strings at compile time and decodes at run time. It is not DEFLATE. | [consteval-huffman](https://code.bitgloo.com/clyne/consteval-huffman) | `summary` |
| In a native PNG decoder, time splits about evenly between inflate and unfiltering. Expect both to matter in the interpreter. | [PNGdec README](https://github.com/bitbank2/PNGdec) | `source` |

## Not confirmed

- emWin's formats and rules. The manual was too large to fetch. Recalled: RLE4/8/16/32 bitmaps drawn from ROM without a buffer. `recalled`
- LZ4 or RLE decode speed and code size on a Cortex-M. No measurement was found.
- Flash size of lodepng, LZ4 or the GIF decoder when built for an MCU. No published number was found.
- The exact rule behind Qt's `Automatic`, and how big a "big transparent border" is. The docs do not say.
- TouchGFX's default formats per framebuffer. A search summary says RGB565 for opaque and ARGB8888 for non-opaque on an RGB565 framebuffer. `summary`
- The compression algorithm Embedded Wizard uses by default. The page does not name it.
- Espressif publishes no decode times. Its speed ratings are words, not numbers.
- LVGL's SVG decoder requirements and RAM use. The doc page was not read.
- The compile-time cost of inflate and unfiltering under GCC 16. It must be measured here; nobody has published it.
- Whether the upstream `v9.6.0` links resolve. The files were read locally only.

## What it means for us

1. The compile-time path should emit plain `lv_image_dsc_t` data that LVGL uses in place. That is the only form with no pixel RAM and no decoder.
2. Four stored formats cover the fast cases: the display's opaque format, RGB565A8, ARGB8888 and A8. All four are drawn without conversion buffers.
3. A8 for single-colour art is the best single optimisation. Slint, Qt and Embedded Wizard all do it. It also makes icons tintable by a design token.
4. The detected colour of an A8 image must travel with it, so the image widget can set the recolor. Otherwise the icon draws in the default colour.
5. Crop the transparent border and keep the original size and offset. Slint's rule (alpha 0 or 1 is transparent) is a tested starting point.
6. Indexed and A1/A2/A4 formats are a size option, never a default. LVGL's software renderer pays for them with heap or with lost transforms.
7. LVGL's RLE and LZ4 are also a size option. They need `LV_BIN_DECODER_RAM_LOAD`, a full-image heap buffer, and a cache to be usable.
8. If size mode is offered, copy TouchGFX's guard: keep the compressed form only when it is below 90% of the raw form.
9. The stride and premultiplied flag must match the configured draw unit. A mismatch makes LVGL silently copy the image to the heap.
10. Align the pixel array to `LV_DRAW_BUF_ALIGN`, and set `data_size` to at least `stride * h`.
11. Dithering should be off by default and on per image. It helps photos and gradients on RGB565, and it hurts every run-length scheme.
12. On desktop the format hardly matters, because LVGL caches a rendered texture. Use ARGB8888 or XRGB8888 and skip GPU texture compression.
13. Photos are the case for form (b): keep the JPEG bytes and decode with tjpgd on the device. Raw RGB565 is the alternative when flash allows.
14. PNG decoding in constant evaluation has no known prior art. Expect to raise `-fconstexpr-ops-limit`, and measure before promising any image size.
15. The choice needs four inputs known at compile time: display colour format, draw unit, stride alignment, and a per-image intent (speed, size, tint, photo).

### Proposed decision rule

Step 0 for every image: decode, crop the transparent border, record original size and offset.

| Image after cropping | MCU, RGB565 display, software renderer | MCU or desktop, 32-bit display | Draw unit that wants premultiplied input |
|---|---|---|---|
| Fully transparent | 1x1 A8 | 1x1 A8 | 1x1 A8 |
| One visible colour, any alpha | A8 plus the colour | A8 plus the colour | A8 plus the colour |
| Fully opaque | RGB565 (swapped if the display is) | XRGB8888 | The display's opaque format |
| Colour with alpha | RGB565A8 | ARGB8888 | ARGB8888 with the premultiplied flag |
| Photo, by user intent | JPEG bytes as they are, or dithered RGB565 | JPEG bytes, or XRGB8888 | The same |

Opt-in overrides, per image:

1. `size`: try I8 or I4 when the image has at most 256 or 16 colours, then RLE. Keep a result only below 90% of the default form.
2. `size` is refused at compile time when the configuration has no RAM load or no cache, with a message naming the missing setting.
3. `transformed` (rotated or scaled at run time): never indexed, never compressed, never line-by-line; keep one of the four direct formats.
4. `dither`: error-diffusion on the colour channels only, for an RGB565 target.
5. `raw`: embed the file's bytes and decode on the device (form b).
