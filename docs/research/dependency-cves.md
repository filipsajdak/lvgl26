# Known-vulnerability sweep for lvgl26, 2026-11-16

Marks: source = primary page read; summary = search summary, page not read; recalled = unverified.
NVD answers were read through WebFetch of services.nvd.nist.gov (keyword search).
So the text is a model-made summary of the API JSON: treat as `source` with that caveat.

## What we build against (read from the repo, source)
- LVGL v9.6.0 tag (cmake/lvgl.cmake); v10 pin is a commit, presumably 93f4c4b21a16f0ae108333066297e3acb48a0179 in build/host-v10/_deps/lvgl-src (rev-parse; that dir is a checkout of the pin).
- Switched on: LV_USE_LODEPNG (LVGL26_PNG, and all tests), LV_USE_TJPGD + LV_USE_FS_MEMFS (LVGL26_JPEG, tests), LV_USE_TINY_TTF (LVGL26_TTF, tests), LV_USE_SNAPSHOT (always), tests only: LV_USE_TEST, LV_USE_FS_STDIO. ThorVG off. FreeType, libpng, libjpeg_turbo, gif, ffmpeg are not enabled.
- Bundled versions: LodePNG 20230410 (lodepng.h), stb_truetype v1.26 "htcw" fork (stb_truetype_htcw.h), TJpgDec R0.03 2021 (tjpgd.c).
- mp-units v2.5.0, gcc:16 image, GCC 16.2.0 source for arm-none-eabi, qemu-system-arm from Debian apt, lizard via pip (unpinned), actions/checkout@v4, upload-artifact@v4, upload-pages-artifact@v3, deploy-pages@v4 (tags, not shas).

## Findings
| Component | Version | Id | What | Fixed in | Affected | Mark |
|---|---|---|---|---|---|---|
| LVGL | 9.6.0 / v10 pin | none | GitHub page lvgl/lvgl/security/advisories: "There aren't any published security advisories". NVD keyword "lvgl": 0 results. Web search for 2026 LVGL CVEs: none | n/a | no known | source (GH, NVD), summary (search) |
| LodePNG | 20230410 | CVE-2026-112020 | Heap out-of-bounds read in lodepng::getFilterTypesInterlaced() (C++ API, lodepng.cpp) from missing bounds checks on decompressed IDAT; NVD says "through 20261006"; published 2026-10-18; no fixed version stated | not stated | Probably not: the function is in the C++ API; LVGL bundles only the C core (lodepng.c) and we do not build it. Not confirmed by reading LVGL's lodepng.c | source (NVD summary); applicability = my inference |
| LodePNG | 20230410 | CVE-2019-17178 | Memory leak in HuffmanTree_makeFromFrequencies (encoder), through 2019-09-28 | after 2019 | No: older than 20230410 and encoder side (we only decode) | source (NVD) |
| LodePNG | 20230410 | CVE-2022-44081 | Segfault in pngdetail (a demo tool) in 20220717 | not stated | No: demo program, not the library | source (NVD) |
| stb_truetype | 1.26 (htcw fork) | CVE-2026-5314, CVE-2026-5315 | Out-of-bounds read in stbtt_InitFont_internal, stbtt__buf_get8 on crafted TTF, up to 1.26 | not stated | Unknown-to-yes in principle: LVGL TinyTTF parses font bytes with this code. In lvgl26 the font is a program-embedded array (scaled_font.hpp:8-11, 58-65), so an attacker needs to control the firmware's font. Not reachable from untrusted input through our API. A program that passes lv_tiny_ttf_create_data/file itself with downloaded fonts is exposed | source (NVD summary) |
| stb_truetype | 1.26 | CVE-2026-18497 | Heap overflow in stbtt__GetGlyphShapeTT on malformed TTF, v1.26, status Deferred, published 2026-08-07 | not stated | same as above | source (NVD summary) |
| stb_truetype | 1.26 | CVE-2022-25514/-25515/-25516 | Heap overflows via ttUSHORT, ttULONG, stbtt__find_table on malformed font; DISPUTED (library says trusted input only) | not stated | same as above | source (NVD summary) |
| stb_truetype | <=1.22 | CVE-2020-6617..6623 | CFF/table over-reads and assertions | after 1.22 | No: we have 1.26 (htcw fork may carry its own differences; unchecked) | source (NVD summary) |
| TJpgDec | R0.03 (2021) | none found | NVD keyword "tjpgd": 0 results | n/a | no known. Absence of CVEs is not evidence of safety: decoder is fed attacker JPEGs only if the program decodes untrusted files | source (NVD) |
| mp-units | 2.5.0 | none found | NVD keyword "mp-units": 0 results; header-only unit library | n/a | no | source (NVD) |
| tj-actions/changed-files | n/a | CVE-2025-30066 | Supply-chain compromise; version tags were rewritten to malicious code; patched v46.0.1 | n/a | No: we do not use that action. Lesson: our actions are pinned to tags (checkout@v4 etc.), not full shas | summary (search); the workflows read: source |
| actions/checkout, upload-artifact, upload-pages-artifact, deploy-pages | v4/v4/v3/v4 | none found | search found no advisory for actions/checkout v4 | n/a | no known | summary (thin) |
| QEMU (Debian qemu-system-arm) | distro version unknown | CVE-2026-3195 (virtio-snd), CVE-2026-3842 (guest write beyond memory), CVE-2026-2243 (VMDK read) | General QEMU flaws; no result naming Cortex-M or the mps2-an386 machine | per distro | No in practice: nightly runs our own firmware (tools/arm-run.sh: mps2-an386, -nographic) with no virtio-snd, no disk image, no network | summary (search; pages not read) |
| lizard (pip, unpinned) | latest at image build | none found | NVD keyword search returned 0 results for lizard (the keyword "lizard python"; a weak query) | n/a | no known; unpinned pip install is a supply-chain exposure (typosquat/compromised release), not a CVE | source (NVD, weak query) |

## Reachability from lvgl26 (source: our headers)
- include/lv/encoded_image.hpp: the file is an embedded constexpr array. Lines 53-54, 69-77, 89-98 parse PNG/JPEG headers in consteval.
  That runs at compile time.
  So an invalid file fails the build and cannot overflow on the device.
  The function make_encoded (line 112-124) hands the program's own bytes to LVGL's decoder (LodePNG/TJpgDec) at run time. The decoders see only the bytes the developer embedded.
  The CVE classes (OOB read on malformed PNG/JPEG/TTF) are reachable only if the program ships an attacker-chosen picture or font.
- include/lv/scaled_font.hpp: same, a `const unsigned char (&)[N]` array (lines 58-65); calls lv_tiny_ttf_create_data.
- lv::write_png / snapshot (png.hpp) encodes our own screen; no parsing of foreign data.
- LV_USE_FS_STDIO is on only in tests (cmake/lvgl.cmake lines 77-79), so no file path from outside reaches LVGL in a shipped build unless the program sets it via LVGL26_LVGL_DEFINITIONS.
- Not checked: whether a program can reach lv_image_set_src with a runtime path or buffer through other lvgl26 headers (the other reviewer owns our code).
  That is the one way the decoder CVEs become reachable.

## Could not check
- GitHub Security Advisories were fetched once; OSV.dev, cve.org, GHSA database pages for lodepng/stb/TJpgDec were not read. NVD keyword search only finds CVEs whose text names the product.
  A CVE filed against "lvgl" under another name (e.g. a vendor fork) would be missed.
- LVGL v9.6.0 release notes and any v9.6.x or later security fixes were not read (docs/research says v9.6.0 is the last v9 release).
- The v10 pin's sha was not confirmed from CMakePresets.json (grep only showed preset names).
  The value 93f4c4b2... is the HEAD of build/host-v10/_deps/lvgl-src and equals the commit hash prefix style of our repo HEAD 93f4c4b: SUSPECT.
  That is lvgl-cpp's own HEAD, not LVGL's; the rev-parse ran in the cwd repo because the lvgl-src dir is not a git checkout.
  Treat the v10 sha as unknown.
- Whether LodePNG's bundled copy in LVGL includes lodepng.cpp (no: only lodepng.c seen listed; not verified by file listing).
  Also whether the htcw stb_truetype fork fixed the 2026 CVEs.
- GCC 16 / libstdc++ advisories, gcc:16 base image CVE scan, Debian apt package CVEs, Python/pip transitive deps, QEMU version actually installed: not searched.
- CVE-2026-112020 numbering looks unusual; only the NVD summary was seen.
