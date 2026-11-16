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
