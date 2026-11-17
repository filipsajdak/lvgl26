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
