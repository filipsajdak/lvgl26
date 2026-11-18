# Targets beyond a GCC host, and functional safety

Can lvgl26 say it runs in a browser, on a phone and in Android Automotive OS, and can it be used in a product with a functional safety goal?
Researched on 2026-11-18. Marks follow [README.md](README.md).
This continues [web-target.md](web-target.md) and [web-emulator.md](web-emulator.md).

## Short answers

- Every target meets one wall first: only GCC 16 compiles lvgl26.
- The owner wants an lvgl26 application built for WebAssembly, running in the page itself. The device's firmware in an emulator is not that goal.
- A native build needs a GCC that writes WebAssembly. GCC's trunk has none. A third-party GCC 17 does, and LLVM's linker links its objects: worth a spike.
- The firmware in our small emulator works today as a demo. That work is on the branch `feat/web-emulator` and is not on `main`.
- Android and Android Automotive OS look reachable with a GCC 16 cross compiler for Android. Nobody here has tried it.
- iOS has no GCC route. It waits for reflection in Clang, or shows the firmware in the emulator.
- lvgl26 cannot be certified for functional safety now, and no page may say "certified", "ASIL" or "SIL" about it.
- Two routes make lvgl26 usable in a product that has a safety goal. Both are below, and neither is built.

## The one constraint

| Target | The compiler the platform ships | A route with GCC 16 | State | Mark |
|---|---|---|---|---|
| Linux on x86-64 and Arm | GCC | Direct | Works, in CI on every push | `source` (`.github/workflows/ci.yml`) |
| Arm Cortex-M | Often an older GCC, or IAR, Keil, Green Hills | `tools/arm-toolchain.sh` builds a GCC 16 cross compiler | The slice runs on a Cortex-M4 in QEMU every night. No real board. | `source` (`tools/arm-run.sh`, `site/status.html`) |
| Browser | Emscripten, which is Clang | None in a GCC release | The firmware in an emulator, on a branch | `measured` ([web-emulator.md](web-emulator.md)) |
| Android, Android Automotive OS | The NDK, Clang only | Community GCC 16 toolchains for Android | Not tried | `summary` |
| iOS | Apple Clang | None found | Not tried | `recalled` |

A second compiler for the same code is the common need behind every row but the first.
It would also serve a team whose microcontroller toolchain is not GCC 16.

## WebAssembly

| Finding | Link | Mark |
|---|---|---|
| No GCC release has a WebAssembly back end. A search on 2026-11-18 found no merge of the patch series. The Steering Committee approved the direction only. | [Phoronix](https://www.phoronix.com/news/GCC-Steering-WebAssembly) | `summary` 2026-11-18 |
| A downstream project pins an experimental GCC 17 that writes wasm32 for WASI Preview 1. Exceptions, RTTI, threads, dynamic linking and SIMD are unsupported there. | [pull request](https://github.com/seo-rii/wasm-llvm/pull/57) | `summary` 2026-11-18 |
| Upstream Clang has no reflection in a release that the search found. Work on P2996 library support was active in the LLVM tracker in October 2026. | [LLVM issue 148123](https://github.com/llvm/llvm-project/issues/148123) | `summary` 2026-11-18 |
| Bloomberg's fork is still the most complete Clang with reflection. | [repository](https://github.com/bloomberg/clang-p2996) | `summary` 2026-11-18 |
| Our emulator runs the unchanged Cortex-M4 firmware in a browser. It is 22.8 KB of WebAssembly, and its pictures equal QEMU's byte for byte. | [web-emulator.md](web-emulator.md) | `measured` 2026-11-16 |
| The branch `feat/web-emulator` holds the emulator, its tests against QEMU, a nightly comparison and a live device on the front page. It is 4 commits ahead of `main` and 317 behind. No pull request exists. | `git rev-list --left-right --count origin/main...feat/web-emulator` | `measured` 2026-11-18 |

### The owner's requirement: a native application (2026-11-18)

The owner asked for lvgl26 applications that run in the browser, not for a device emulated there.
His idea: compile the application to a library with GCC, and link it into WebAssembly with LLVM's tools.

| Finding | Link or how checked | Mark |
|---|---|---|
| A library that GCC builds for a processor holds that processor's machine code. LLVM's WebAssembly linker links WebAssembly objects only, and no linker translates machine code. So the idea fails as long as GCC writes native code. | [web-target.md](web-target.md), the two-stage section | `source` 2026-11-17 |
| GCC's trunk is version 17.0.0 and has no `gcc/config/wasm` directory, and the mirror has no branch named for it. The back end is not merged. | `gh api repos/gcc-mirror/gcc/contents/gcc/config/wasm` (404), `gcc/BASE-VER` | `source` 2026-11-18 |
| A third-party project merged a "pinned GCC 17 WebAssembly backend" on 2026-10-26. Its description: GCC's own C and C++ front ends write annotated WebAssembly text, WABT assembles relocatable objects, and "external LLD links" them for WASI Preview 1, with libc and "genuine GNU libstdc++". | [pull request 57](https://github.com/seo-rii/wasm-llvm/pull/57), its description read | `source` 2026-11-18, the project's own claim |
| The same description reports ABI checks against objects that Clang built, and 539 programs compiled and run in a Chromium worker. An earlier summary lists exceptions, RTTI, threads and dynamic linking as unsupported. | same; [search summary](https://github.com/seo-rii/wasm-llvm/pull/57) | `source` for the first, `summary` for the second |
| lvgl26 builds with `-fno-exceptions -fno-rtti`, and its core needs no thread. | `tests/no_exceptions.cpp`, `tools/arm-run.sh` | `source` |

What it means:

1. **The idea works in one form.** GCC must write WebAssembly objects, not native ones. Then LLVM's linker can join them with LVGL, which Emscripten builds from C.
2. **That GCC exists outside GCC.** The upstream back end is not merged. The third-party one is experimental, pinned to one GCC 17 commit, and unverified by us.
3. **It fits what we need.** What it lacks (exceptions, RTTI, threads) is what lvgl26 does not use.
4. **Not known:** whether that GCC 17 commit compiles our reflection code, what the ABI between its objects and Emscripten's LVGL does with our structs, and how fast it compiles.
5. **The spike:** build that compiler, compile the slice to a WebAssembly object, link it with LVGL built by Emscripten, and draw LVGL's buffer on a canvas.
6. The firmware in the emulator stays what it is: a demo of the device's own binary, on `feat/web-emulator`.

## Android and Android Automotive OS

| Finding | Link | Mark |
|---|---|---|
| The NDK removed GCC in r18 (2018). It ships Clang and LLVM only. | [NDK revision history](https://developer.android.com/ndk/downloads/revision_history) | `summary` 2026-11-18 |
| A community guide builds C programs for Android with a GCC 16.1.1 cross compiler for `aarch64-unknown-linux-android`, with the API level in the target name and the NDK's sysroot. | [XDA guide](https://xdaforums.com/t/guide-how-to-compile-a-c-program-for-android-using-a-recent-gcc-cross-compiler-in-linux-macos-or-windows.4790485/) | `summary` 2026-11-18 |
| The search found no official Android port of LVGL. A community port of LVGL 8 exists. | [android-lvgl](https://github.com/fbiego/android-lvgl) | `summary` 2026-11-18 |
| An application with native code needs no Java: the NDK's own sample is a native activity. | [NDK sample](https://developer.android.com/ndk/samples/sample_na) | `summary` 2026-11-18 |
| In QNX's reference design the instrument cluster runs in the hypervisor's host, and Android Automotive OS is a guest for infotainment. A safety-certified product needs the hypervisor's safety variant. | [QNX documentation](https://www.qnx.com/developers/docs/7.1/com.qnx.doc.qavf.overview/topic/overview.html) | `summary` 2026-11-18 |
| Google's platform for software-defined vehicles lists a "Display Safety" part: functionally safe, early-boot rendering paths for cluster information and telltales. | [Android documentation](https://source.android.com/docs/automotive/sdv) | `summary` 2026-11-18 |
| Third-party applications for Android Automotive OS are built from templates, not from free drawing. | none | `recalled` |

What it means:

1. Android Automotive OS is Android. A route for Android is a route for it.
2. The place for lvgl26 there is the manufacturer's own screens, which may hold native code. A store application from a template is not.
3. The route to try: build the native library with a GCC 16 cross compiler for Android, and draw LVGL's buffer into the window of a native activity.
   It needs a display driver, touch input and a build recipe. It is a spike of days, not a claim.
4. The cluster with its telltales is usually outside Android. That is the functional safety question below.

## iOS

| Finding | Link | Mark |
|---|---|---|
| The search found no official iOS port of LVGL. Swift wrappers exist, and forum users call LVGL workable there but not native in feel. | [LVGLSwift](https://github.com/padl/lvglswift), [forum](https://forum.lvgl.io/t/what-are-the-pros-and-cons-of-using-littlevgl-with-iphone-apps/18839) | `summary` 2026-11-18 |
| No GCC that targets iOS was found. Apple's toolchain is Clang. | none | `recalled` |

iOS waits for reflection in Clang, and then for Apple's Clang to take it.
Until then a phone can show the firmware in the emulator, which is plain C and builds anywhere.

## Functional safety

| Finding | Link | Mark |
|---|---|---|
| The search found no certification of the open-source LVGL. | [search of 2026-11-18](https://lvgl.io/safe) | `summary` 2026-11-18 |
| LVGL Safe is a separate library, "not a fork": written from scratch in C99, with none of the open-source code carried over. | [README](https://github.com/lvgl/lvgl_safe) | `source` 2026-11-18 |
| It is "source available under a commercial agreement, per product". The open-source LVGL stays MIT. | same | `source` 2026-11-18 |
| It allocates nothing at run time: "the caller owns every struct and buffer". Widgets are created only and live as long as the program. It has no global state. | same | `source` 2026-11-18 |
| It has no cascading styles, themes, flex or grid: explicit fields per widget, colours per state and absolute positions. One software renderer. | same | `source` 2026-11-18 |
| Today it has six widgets: rectangle, label, button, image, image button and arc. It has value binding on labels, translations, and offline converters for images and fonts. | same | `source` 2026-11-18 |
| Its README says "certification ready" and "compatible with", up to ASIL B (ISO 26262), Class B (IEC 62304), SIL 2 (IEC 61508) and DAL C (DO-178C). It does not say certified. | same | `source` 2026-11-18 |
| The full delivery with safety manual, verification report and traceability is due in December 2026. The preview was pushed last on 2026-08-25. | same, and the repository's metadata | `source` 2026-11-18 |
| Qt Safe Renderer is certified by TÜV NORD to ISO 26262 ASIL D. The certificate covers that module only. Safety elements are drawn in a layer over the uncertified main UI. | [Qt blog](https://www.qt.io/blog/2018/05/31/function-safety-certified-qt-safe-renderer), [delivery](https://doc.qt.io/QtSafeRenderer/qtsr-delivery.html) | `summary` 2026-11-18 |
| Slint has a demo of a safety-critical UI on a Cortex-M7. The search found no certification. | [Slint example](https://github.com/slint-ui/slint/tree/master/examples/safe-ui) | `summary` 2026-11-18 |
| MISRA C++:2023 targets C++17. The search found no edition for C++20 or later. | [MathWorks](https://www.mathworks.com/help/bugfinder/misra-cpp-2023-rules-and-directives.html) | `summary` 2026-11-18 |
| A compiler is qualified for one version and one target. For GCC that is a third-party test suite. No kit for GCC 16 was found. | [overview](https://wrocpp.github.io/toolset/qualified-compilers/) | `summary` 2026-11-18, weak source |
| Under ISO 26262-8 clause 11 a tool whose errors are caught with high confidence needs no qualification (TCL1). A code generator whose output is tested as the standard asks can reach that. It is not automatic. | [Synopsys paper](https://www.synopsys.com/content/dam/synopsys/solutions/documents/automotive-tool-confidence-iso-26262-wp.pdf), [itemis](https://www.itemis.com/en/glossary/tool-qualification/) | `summary` 2026-11-18 |

What it means:

1. **lvgl26 itself cannot be certified now.** Its language has no coding standard, and its only compiler has no qualification kit.
2. **The usual architecture does not need a certified UI framework.** The main UI has no safety level. A small certified renderer draws the telltales in a layer above it, or in a partition of its own.
   lvgl26 fits the first part today. This is how Qt is used in a cluster.
3. **LVGL Safe is built the way lvgl26 already goes.** The caller owns every struct and buffer, nothing is allocated, widgets are created once.
   lvgl26 knows a tree's widgets, sizes and styles at compile time, so it could lay out that memory for a program.
   A second back end over LVGL Safe is the route with the most reuse. LVGL Safe is commercial, so that part of a product is outside "no royalties".
4. **A generator route.** GCC 16 would write plain C99 for a screen, and the product's qualified compiler would build it.
   What gets verified is the generated C, which is the argument for TCL1. Logic written in C++26 does not travel. Not tried.
5. **Wording.** The site may say what the compiler checks and what the tests run. It may say a route is being studied. It may not attach a safety level to lvgl26.

## Not confirmed

- Whether a GCC 16 cross compiler for Android builds lvgl26, and whether its objects link in an NDK build. Nobody here tried.
- The licence terms and the price of LVGL Safe, and whether its preview may be linked from an open-source project.
- Whether LVGL Safe's API can express what our trees need: it has no layout, so positions would be computed by us at compile time.
- The text of ISO 26262-8 clause 11 itself. Only summaries were read.
- Every row marked `summary`: a search engine's reading of the page.
- The state of GCC's WebAssembly back end after late October 2026, from GCC's own lists.

## Decisions for the owner

1. **Is the native WebAssembly build a spike now?** Decided in part on 2026-11-18: the goal is a native application, not the emulator. Whether to spike the third-party GCC 17 is open.
2. **Is Android a spike now?** A GCC 16 cross compiler for Android, a native activity and LVGL's buffer in its window.
3. **Which functional safety route is studied first?** Beside a certified renderer, a back end over LVGL Safe, or generated C.
4. **Does the site name the planned targets?** As rows of the status page, or not until one works.
