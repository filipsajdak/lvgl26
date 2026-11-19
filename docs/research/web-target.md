# A browser target

Can an lvgl26 application run in a browser, from the same code? Researched on 2026-11-15. Marks follow [README.md](README.md).

## The obstacle

LVGL itself runs in a browser. lvgl26 does not yet, because of the compiler.

| Fact | Mark |
|---|---|
| LVGL's documented browser route is `lv_web_emscripten`: Emscripten with SDL, built with `emcmake`, giving an `index.html`. [LVGL docs](https://docs.lvgl.io/9.5/integration/pc/browser.html) | `summary` |
| Emscripten is built on Clang. Clang has no C++26 reflection, so it cannot compile lvgl26. | `source` (our CLAUDE.md, "Known tool limits") |
| Bloomberg's `clang-p2996` fork implements reflection experimentally. Its maintainers say not to build production artifacts with it. [repository](https://github.com/bloomberg/clang-p2996) | `summary` |
| Nothing was found on that fork with Emscripten, or on reflection in upstream Clang after mid-2025. | `summary` |

lvgl26 also uses contracts and a GCC builtin for compile-time errors. A Clang route would have to cover those too.

## Routes

| Route | State | What it gives | Mark |
|---|---|---|---|
| GCC's own WebAssembly back end | An RFC patch series of May 2026 adds `config/wasm`, for wasm32 only. The GCC Steering Committee approved the idea. Not merged as far as found. [patch](https://patchwork.sourceware.org/project/gcc/patch/20260505223045.347444-4-feedabl3@gmail.com/), [Phoronix](https://phoronix.com/news/gcc-wasm-webassembly) | A true browser target with our compiler, once it lands | `summary` |
| The firmware in an emulator in the browser | QEMU runs in a browser, built with Emscripten. A community project boots real Pebble firmware (ARM) and draws its display to an HTML canvas. It is slow: the interpreter needs key presses held for a second. [qemu-wasm demo](https://ktock.github.io/qemu-wasm-demo), [QEMU patches](https://patchew.org/QEMU/cover.1744032780.git.ktokunaga.mail@gmail.com) | The same binary that runs on the device, shown on a web page | `summary` |
| Clang with reflection, through Emscripten | Experimental fork only | A true browser target, with a second compiler to support | `summary` |
| Render on a server, stream the picture | Always possible | A demo, not a target | `recalled` |

Not confirmed: which machine models the browser build of QEMU includes. Its demos are Linux guests on x86_64, AArch64 and RISC-V 64. Our slice runs on `mps2-an386`, a Cortex-M machine.

## What it means for us

1. A real WebGL target waits for a compiler: GCC's WebAssembly back end, or reflection in Clang.
2. The emulator route needs no new compiler. It shows the device's own binary, which is a stronger claim than a separate web build.
3. Keep the code ready: nothing in lvgl26 may assume a desktop or an operating system. The freestanding build already checks that.
4. WebGL itself matters only for the 3D showpiece. A flat screen needs a canvas, not WebGL.

## The two-stage idea (2026-11-17)

A colleague asked whether lvgl26 could be built as a static library that Rust, or Emscripten and Clang, then turns into WebAssembly.
This section checks it. Marks follow [README.md](README.md).

| Finding | Link | Mark |
|---|---|---|
| GCC has no WebAssembly target in a release. A static library that GCC builds is native code, so Rust and Emscripten cannot link it into a wasm module. | [GCC back end RFC](https://patchwork.sourceware.org/project/gcc/patch/20260505223045.347444-4-feedabl3@gmail.com/) | `summary` |
| The GCC Steering Committee approved a wasm back end in concept in mid-2026. It covers the direction, not the patches. Initial support could come in GCC 17.1. | [Phoronix](https://www.phoronix.com/news/GCC-WASM-WebAssembly) | `summary` |
| The back end was still unmerged in late October 2026. It lacks reference types, tables, exceptions, debug info and data sections. | [Phoronix](https://www.phoronix.com/news/GCC-WASM-WebAssembly) | `summary` |
| No upstream Clang reflection was found. Bloomberg's `clang-p2996` fork is still the reference, and it says it is experimental and not for production. | [repository](https://github.com/bloomberg/clang-p2996) | `summary` |
| Emscripten accepts another LLVM and Clang through `EM_LLVM_ROOT` or `LLVM_ROOT` in `.emscripten`, built with lld, clang and the WebAssembly target. | [Emscripten docs](https://emscripten.org/docs/building_from_source/configuring_emscripten_settings.html) | `summary` |
| lvgl26 would still need contracts and `template for` from a Clang that has neither. A Clang route is a port of the library, not a build option. | `CLAUDE.md`, "Known tool limits" | `source` |

What a two-stage build can mean:

1. **GCC makes a native library, Rust or Emscripten link it.** This does not work today: the object code is not WebAssembly.
2. **GCC writes portable source, Clang builds it.** Stage one writes plain C or C++20 for the screen.
   It holds the LVGL calls, the styles, the fonts and the pictures, which the compiler already computes as constants. Stage two can be any compiler. Logic written in C++26 does not travel, so only a static screen with bound values would. Not tried.
3. **GCC's own wasm back end.** One stage, one compiler, no Rust. It waits for GCC 17.
4. **The firmware in an emulator.** The route that works today: see [web-emulator.md](web-emulator.md).

A Rust binding of lvgl26 itself is not possible: the library is header-only templates and constant evaluation, with no stable ABI to bind to. The Rust binding of LVGL is a separate note: `docs/research/rust-binding.md`, in its own pull request.

## The native WebAssembly spike (2026-11-19)

The question: can an lvgl26 application run in a browser as WebAssembly that GCC wrote, linked with LVGL that Clang built?
The scripts are in `tools/wasm/`, the program in `examples/wasm/`, on the branch `spike/wasm-native`.
Marks follow [README.md](README.md).

| Finding | Link or how checked | Mark |
|---|---|---|
| The project seo-rii/wasm-llvm pins an experimental GCC 17.0.0, commit ac20dcd5 of 2026-05-28, plus 11 patches. Its licence is MIT. | [pull request 57](https://github.com/seo-rii/wasm-llvm/pull/57) | `source` 2026-11-19 |
| That GCC builds on aarch64 Linux in a container as a plain cross compiler for wasm32-wasi. It needs the arm64 WASI SDK 33 and a WABT fork. | `tools/wasm/build-gcc.sh` | `measured` 2026-11-19 |
| With that commit, `#include <lv/lv.hpp>` fails. Reflection of the global namespace and splices in templates are bugs of that snapshot. | `members_of(^^::)`: "use of built-in parameter pack '__integer_pack' outside of a template" | `measured` 2026-11-19 |
| The back end and its patches add about 3,900 lines, about 60 of them in core files of GCC. It applies to GCC trunk of 2026-11-19 (e632e263) with no rejects. | `tools/wasm/fetch-gcc.sh` | `measured` 2026-11-19 |
| On that trunk `#include <lv/lv.hpp>` compiles to a wasm object in 2.9 s. The slice compiles at -O1 and -O2. | `tools/wasm/wasm-g++` | `measured` 2026-11-19 |
| At -Os the front end stops: "taking address of an immediate function" in `std::optional<std::meta::data_member_options::_Name>`. | `examples/mcu/slice.cpp` with -Os | `measured` 2026-11-19 |
| libstdc++'s `noop_coroutine` frame holds `void(*)()` pointers and calls them as `void(*)(void*)`. WebAssembly traps on that call. GCC 16.2.0 has the same lines (GCC bug 128871). | `detail::idle()` in `lv/task.hpp`: an own frame with functions of the right type. The header patch `tools/wasm/fix-coroutine.sh` is no longer needed. | `measured` 2026-11-19 |
| LVGL v9.6.0 compiled by the WASI SDK's Clang links with the GCC objects through `wasm-ld --fatal-warnings`. The linker reports no signature or layout problem. | `tools/wasm/build-lvgl.sh` | `measured` 2026-11-19 |
| The unchanged slice runs under Node's WASI and prints `ok`. | `tools/wasm/run.mjs` | `measured` 2026-11-19 |
| The frame of the slice screen is 153,600 bytes. The WebAssembly build and the host build (GCC 16) write the same bytes. | `cmp`, SHA-256 4b0fac4a | `measured` 2026-11-19 |
| Headless Chrome runs the module in a page, draws the frame on a canvas, and computes the same SHA-256. | `tools/wasm/page/index.html` | `measured` 2026-11-19 |

Sizes and times of the first version of the example, a still frame, with GCC at -O2 and LVGL at -Os:

| What | Value | Mark |
|---|---|---|
| The module, with names | 765,723 bytes raw, 182,974 bytes with Brotli | `measured` 2026-11-19 |
| The module, stripped | 630,901 bytes raw, 142,761 bytes with Brotli | `measured` 2026-11-19 |
| The compile of the file by `cc1plus` on the host (GCC 16) | 79.9 billion instructions | `measured` 2026-11-19 |
| The compile of the same file for WebAssembly | 7.8 s user in the container; no instruction counter there | `measured` 2026-11-19 |
| Node: compile 1.2 ms, instantiate 0.5 ms, run 0.5 ms when warm. The first run takes 6 ms. | `tools/wasm/run.mjs` | `measured` 2026-11-19 |

### An interactive application (2026-11-19)

The owner asked for an application that keeps running in the page, with pointer input.
`examples/wasm/counter/app.hpp` is written with lvgl26 only: a tree, states, bindings, handlers and a task.
`examples/wasm/platform.cpp` gives it a clock, a pointer, a frame and one step.

| Finding | Link or how checked | Mark |
|---|---|---|
| GCC ignores `__attribute__((export_name))` for this target. The linker exports work: `crt1-reactor.o`, `--no-entry`, `--export=name`. | `tools/wasm/link-demo.sh` | `measured` 2026-11-19 |
| `lv::application::run()` blocks, so the page calls `step()` itself with an injected clock. No library change was needed. | `examples/wasm/platform.cpp` | `measured` 2026-11-19 |
| lvgl26 has no input device type. The platform file calls LVGL's C functions for a pointer. A type `lv::pointer` would remove them. | `examples/wasm/platform.cpp` | `source` 2026-11-19 |
| A scripted run gives one last frame of 691,200 bytes. The host build, the module under Node and the module in Chrome agree on its SHA-256. | `tools/wasm/replay.mjs`, `host_main.cpp`, `#selftest` | `measured` 2026-11-19 |
| Real mouse events from the DevTools protocol move the dial, the slider and the switch. A task ticks on its own. | `tools/wasm/drive-chrome.mjs` | `measured` 2026-11-19 |
| The page steps at 60 frames a second. A step with its canvas draw takes 0.06 to 0.07 ms. | headless Chrome | `measured` 2026-11-19 |
| A press reaches a changed frame in 14.6 to 16.3 ms for four of five presses, and in 65.7 ms for the fifth. | headless Chrome | `measured` 2026-11-19 |
| The module is 1,195,876 bytes, and 151,441 with Brotli. The frame buffer is stored as zeros in the data segment. | `wc`, `brotli -q 11` | `measured` 2026-11-19 |
| LVGL samples input every 33 ms, so a press shorter than that can be missed. The page holds a press for 50 ms. | `tools/wasm/page/index.html` | `measured` 2026-11-19 |

Not tried: keyboard, the wheel, high-DPI scaling, resizing, more than one display, and touch on a phone.

### The bouncing ball (2026-11-19)

The owner asked for the bouncing ball of the site's "Motion, written as a coroutine" as the test of coroutines.
The scene was removed from `main` in commit 506e60e and is in history as `examples/window/bounce/`.
`examples/wasm/bounce/scene.hpp` is that file, with the SDL window, the recorder and `main` left out.
The tree, the tokens and the five coroutines are unchanged.

| Finding | Link or how checked | Mark |
|---|---|---|
| The scene builds against today's library with no change. The platform file of the page gives it the display and the clock. | `examples/wasm/bounce/app.hpp` | `measured` 2026-11-19 |
| A script of 8 s in 16 ms steps from a clock the test controls gives 14 frames, 8,601,600 bytes. The host build and the module agree byte for byte, at -O1 and -O2. | `tools/wasm/replay.mjs`, `cmp` | `measured` 2026-11-19 |
| The same bytes include the shadow and its blur, which LVGL's software renderer draws. | same | `measured` 2026-11-19 |
| The first run trapped with "memory access out of bounds" in the task `shape`. The default stack of 64 KiB is too small. | stack sizes 160 KiB failed, 192 KiB passed | `measured` 2026-11-19 |
| One `.set({...})` on a widget needs well over 100 KiB of stack, because its temporary holds every property. | same | `measured` 2026-11-19 |
| The module is linked with a stack of 1 MiB. | `tools/wasm/link-demo.sh` | `measured` 2026-11-19 |
| Chrome ran the page for 70 s with no trap: 3,960 frames drawn at 60 frames a second, 4,267 coroutine resumes, LVGL's heap between 7.8 and 8.8 KiB. | `tools/wasm/drive-chrome.mjs --run`, `count-resumes.cpp` | `measured` 2026-11-19 |
| A resume is a call of `lv::detail::resume_task`. The counting build counts them with `-finstrument-functions`. Node counted 4,163 in 62 s of clock. | `tools/wasm/count-resumes.cpp` | `measured` 2026-11-19 |
| A step with its draw takes 0.2 ms. A normal Chrome window on macOS also gave 60 frames a second. | headless and headed Chrome | `measured` 2026-11-19 |

### The nightly check (2026-11-19)

The nightly run builds the two applications for WebAssembly and checks them.
It is an experimental target, and the pull request run does not touch it.

| Finding | Link or how checked | Mark |
|---|---|---|
| The image `.github/ci-image/wasm/Dockerfile` fetches every input at a commit or against a SHA-256. Nothing of the compiler or the back end is in the repository. | the Dockerfile | `source` 2026-11-19 |
| Pins: GCC trunk e632e263, the back end's recipe at 91e8e645, the back end as one patch with a SHA-256, WASI SDK 33, WABT, autoconf 2.69, Node 26.13.1, Ubuntu 24.04 by digest. | the Dockerfile | `source` 2026-11-19 |
| The image builds in 9 min 41 s on an 8-core arm64 virtual machine, and is 1.8 GB. | `docker build` | `measured` 2026-11-19 |
| On a GitHub runner (`ubuntu-latest`, x86-64) the image builds in 27 min 18 s. The check in it takes 46 s. The job limit is six hours. | one run of a throwaway workflow, 2026-11-19 | `measured` 2026-11-19 |
| In that image both applications build and give the host's frames. The check takes 1 min 16 s. | `docker run` of the steps of the job | `measured` 2026-11-19 |
| The job compares the frames of each application with the host build, then runs the pages in Chrome: the counter with real clicks, the ball for a minute. | `.github/workflows/nightly.yml` | `source` 2026-11-19 |
| The licences: the compiler is GPL-3.0-or-later with runtime exceptions, the SDK Apache-2.0 with LLVM exceptions, WABT Apache-2.0, the back end's project MIT. Nothing of them is shipped. | the Dockerfile header | `source` 2026-11-19 |

What is not verified:

- Whether the -Os error also occurs on a native build of the same trunk. It is in the front end.
- Anything with threads, exceptions or RTTI. The target does not support them, and lvgl26 uses none.
- Tests beyond one frame. The test suite of lvgl26 was not run on this target.
- The patches of the project are under GPL as part of GCC. The WABT fork and the scripts are separate.

What it means:

1. **The idea works.** Native lvgl26 code runs in a browser, with no emulator and no second compiler for our code.
2. **The compiler is the risk.** It is a patch set on GCC trunk, not a release. It needs a rebase on every GCC change.
3. **Wait or adopt.** Upstream support in GCC 17 would remove the patch set. Until then the target is experimental.
