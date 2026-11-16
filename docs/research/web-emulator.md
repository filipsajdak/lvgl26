# A small emulator for the browser

Can the lvgl26 firmware draw in a browser through a small CPU emulator compiled to WebAssembly, instead of QEMU? Tested on 2026-11-16. Marks follow `docs/research/README.md`. This continues `docs/research/web-target.md`.

Short answer: yes. The unchanged Cortex-M4 firmware (`slice.elf`, `live.elf`) runs in a 970-line interpreter that is 22.8 KB as WebAssembly. Its pictures equal QEMU's and the host build's, byte for byte.

## The firmware's instruction set

| Finding | Link or how measured | Mark |
|---|---|---|
| The firmware is ARMv7E-M, Thumb-2 only, with no FPU instruction. The toolchain file passes `-mcpu=cortex-m4 -mthumb` and no float ABI, so floats are software. | `arm-none-eabi-readelf -A slice.elf` (`Tag_CPU_arch: v7E-M`, no `Tag_FP_arch`); `cmake/arm/toolchain.cmake` | `measured` |
| It uses the usual Thumb-2 integer set plus a few DSP instructions: `smulbb`, `smlabb`, `uadd8`, `sel`, `uxtah`, `sxtah`, and `sdiv`, `udiv`, `tbb`, `tbh`, `bfi`, `ubfx`, `clz`, `rbit`, `rev16`, `umull`, `smull`, `smlal`, `umlal`. | `arm-none-eabi-objdump -d`, mnemonics counted (`slice.dis`, `live.dis`) | `measured` |
| Of the system it needs: the vector table, SysTick with its interrupt, `wfi`, and `bkpt 0xAB` (semihosting) for the console, the frame file and the pointer file. | `cmake/arm/startup.c`, `examples/mcu/live.cpp` | `source` |
| Flash image 260 KB (`live.elf` as a raw binary: 259,764 bytes; 177 KB with gzip, 160 KB with Brotli). | `arm-none-eabi-objcopy -O binary`, `gzip -9`, `brotli` | `measured` |

## Survey of small emulators

| Emulator | What it is | Licence | Fit | Mark |
|---|---|---|---|---|
| [libmicroemu](https://github.com/chgroeling/libmicroemu) | ARMv7-M (Cortex-M4 without FPU) in C++, with SysTick and semihosting. No NVIC yet. Its README puts performance work after completeness. About 22,700 lines. | MIT | The closest existing one. It includes `<string_view>`, `<array>`, `<iostream>` and others. A WebAssembly build therefore needs a C++ standard library for wasm32 (wasi-sdk or Emscripten). Neither is on this machine. Not built. | `source` (LICENSE, README and includes read in a clone, commit `de017a3`) |
| [zmu](https://github.com/jjkt/zmu) | Cortex-M0/M3/M4 emulator in Rust. Last push July 2022. | Apache-2.0 per the search summary | Would need the Rust wasm32 target. Not tried. | `summary` |
| [thumbulator](https://github.com/dwelch67/thumbulator) | Thumb-1 (ARMv6-M) interpreter in C. | Not checked | Too little: the firmware needs Thumb-2. | `recalled` |
| [rp2040js](https://github.com/wokwi/rp2040js) | Cortex-M0+ (RP2040) in TypeScript. | MIT | ARMv6-M only. | `recalled` |
| [thumb2ISS](https://pypi.org/project/thumb2ISS/) | Cortex-M4 instruction set simulator in Python. | Not checked | Python: not for a page. | `summary` |
| [Unicorn](https://github.com/unicorn-engine/unicorn) | QEMU's CPU cores as a library. | GPL-2.0 | Megabytes, and the same licence question as QEMU. | `recalled` |
| [mini-rv32ima](https://github.com/cnlohr/mini-rv32ima) | RV32IMA in one C header of a few hundred lines. | MIT | Small and easy to build freestanding. Needs a RISC-V firmware: see below. | `recalled` |
| [TinyEMU](https://bellard.org/tinyemu/) | RISC-V 32/64 with FPU, by Fabrice Bellard; runs in browsers. NuttX with LVGL has been shown in it. [article](https://lupyuen.codeberg.page/articles/tinyemu) | MIT | Larger than needed; same toolchain need. | `summary` |
| [rv32emu](https://github.com/sysprog21/rv32emu), [libriscv](https://github.com/libriscv/libriscv) | RV32 interpreters, the second with a fast decoder cache. | MIT, BSD-3-Clause | Same toolchain need. | `recalled` |

The RISC-V route was not built. It needs a second firmware build, and this machine has no RISC-V GCC (`riscv64-unknown-elf-gcc`, `riscv64-elf-gcc`, `riscv32-unknown-elf-gcc`: none on PATH, `measured`). It would take a `riscv32` GCC 16 cross compiler, then newlib and libstdc++ built as `tools/arm-toolchain.sh` does for Arm. That is a large install, which the spike's limits exclude.

The search found no small C emulator for Thumb-2 under a permissive licence that builds without a C++ library. So the spike wrote one: `emu/m4.c`.

## What was built

All files are in a local scratch directory `wasm-small/`, outside the repository. Nothing in the repository was changed.

| File | What it is |
|---|---|
| `emu/m4.c` | The interpreter: 970 lines of freestanding C. Thumb-2 integer instructions, the DSP ones listed above, SysTick, exception entry and return, `wfi`, semihosting passed to the host, and a small board of its own (frame buffer at `0x60000000`, flush, pointer and clock registers at `0x40000000`). |
| `emu/build.sh` | `clang --target=wasm32 -O3 -nostdlib -ffreestanding`, linked by `wasm-ld`. No Emscripten, no wasi-libc, no C library. |
| `emu/host.mjs` | The host side, 4.4 KB, shared by Node and the page: ELF loader, the five imports, the frame-stream reader. |
| `emu/run.mjs`, `emu/live.mjs`, `emu/bench.mjs` | Node runners: a firmware to its end; the never-ending one in real time; the redraw benchmark. |
| `emu/make-page.py`, `page.html` | One self-contained HTML file (551 KB) with the emulator, `live.elf` and a canvas. |
| `fw/bench.cpp`, `fw/CMakeLists.txt` | A firmware for the emulator's own board: LVGL in direct mode into the mapped frame buffer, clock from a register. Built in `fw-arm/`. The same CMake file builds `examples/mcu/slice.cpp` for the host in `fw-host/`. |
| `fw-qemu/examples/mcu/slice.elf`, `live.elf` | The repository's firmware (worktree `build/wt/web`, preset `arm-m4` settings), built unchanged. |
| `out-*.png`, `out-slice-m4.bin` | The pictures and the frame stream. |

Two ways for pixels to leave were tested. The unchanged firmware writes `frames.bin` through semihosting, as for QEMU; the emulator hands each write to JavaScript. The bench firmware draws into memory the host reads directly.

## What we measured

Node 26.8.2 (V8), Apple silicon Mac (arm64), Homebrew clang 23.1.5. The machine was heavily loaded by other builds during every run (load average 240 to 275).
So times vary by a factor of three between runs. Instruction counts are exact and repeat.

| Finding | How measured | Mark |
|---|---|---|
| `slice.elf` runs to `ok` and exit code 0. Its console output equals QEMU's. | `node emu/run.mjs fw-qemu/examples/mcu/slice.elf` | `measured` |
| The frame stream (184,472 bytes, 4 frames, 33 areas) is identical to native QEMU 's. | `cmp qemu-run/frames.bin out-slice-m4.bin` | `measured` |
| The same `slice.cpp` built for the host with `g++-16` writes the identical frame stream. Arm in the emulator and the host build draw the same pixels. | `cmp host-run/frames.bin out-slice-m4.bin` | `measured` |
| `m4.wasm` is 22,823 bytes: 7,037 with `gzip -9`, 6,187 with `brotli -q 11`. QEMU's is 53.5 MB and 10.0 MB with Brotli. | `ls -l`, `gzip`, `brotli` | `measured` |
| The emulator is ready 1.3 to 6.7 ms after the process has the bytes. | `run.mjs`, from before `WebAssembly.instantiate` to after reset | `measured` |
| First frame of `slice.elf`: 41 to 131 ms after start (2,346,558 instructions). QEMU in the browser: 1.4 to 1.6 s from page load. | `run.mjs`, six runs | `measured` |
| Speed: 25 to 90 million instructions a second, depending on machine load and on V8's compiler tier. 71 to 76 million with `node --no-liftoff` in a calmer moment. | `bench.mjs` | `measured` |
| Full redraw, 320 x 240: 637,274 instructions. 7 to 36 ms, so 27 to 140 frames a second. QEMU in the browser: about 0.4 s for mount plus first frame. | `bench.mjs`, 60 redraws, six runs | `measured` |
| Mount plus first frame, 320 x 240, direct mode: 1,097,750 instructions, 19 to 83 ms. | `bench.mjs` | `measured` |
| Full redraw, 800 x 480 (same widgets, larger background): 808,697 instructions, 28 ms, 35 frames a second under load. | `bench.mjs fw-arm/bench.elf 800 480`, two runs | `measured` |
| Small change, the dial moves (1,251 pixels flushed): 368,446 instructions, 5 to 15 ms, 66 to 198 frames a second. | `bench.mjs`, 300 changes | `measured` |
| Small change, a label's text (1,587 pixels): 144,397 instructions, 2 to 8 ms, 127 to 490 frames a second. | `bench.mjs`, 300 changes | `measured` |
| `live.elf` in real time, unchanged: 27.5 to 28.0 frames a second (the firmware's limit is 30), median gap 33 to 34 ms. The emulator is busy 13 to 20 % of one core. A run under the worst load gave 22.3. QEMU in the browser: 25 to 28. | `node emu/live.mjs fw-qemu/examples/mcu/live.elf 10 --press 60 197`, four runs | `measured` |
| A press on the button is handled 4 to 23 ms after release, and the firmware prints `pressed`. | `live.mjs --press` | `measured` |
| The page draws in headless Chrome 157 from a `file://` address: one HTML file, no server, no HTTP headers, no threads. | `page.html`, screenshot `out-page.png` | `measured` |
| V8 does not swap a running WebAssembly function for its optimized version. One long `emu_run` call stayed in the baseline tier at 25 to 40 million instructions a second. Short calls (200,000 instructions) and a separate `step` function fix it. | `bench.mjs` with a 50,000,000 budget against 200,000, and `--no-liftoff` | `measured` |

## Not confirmed

- Frame rate and timing inside a browser. Headless Chrome took the picture at page load; the page's own counters were not read. Only Node gave numbers.
- Other browsers, a phone, a static host. Nothing in the design needs headers or threads, but only Chrome from a file was run.
- The emulator's correctness beyond this firmware. The evidence is three byte-identical outputs. It was not run against an instruction test suite. Instructions the firmware does not use are missing or untested: the FPU, saturating arithmetic other than `ssat` and `usat`, most parallel adds, exclusive byte and halfword access.
- No NVIC beyond SysTick, no process stack (PSP), no fault exceptions, no MPU. An RTOS would need the first two.
- Times on a quiet machine. Every number was taken under a load average above 240.
- libmicroemu's speed and size as WebAssembly. It was read, not built.
- Licences marked `recalled` or "not checked" in the survey.

## What it means for us

1. The browser demo does not need QEMU. A 6 KB download (Brotli) replaces a 10 MB one.
   It runs the same `.elf`, starts in milliseconds, and needs no cross-origin headers, so a static host such as GitHub Pages can serve it.
2. It is faster where QEMU's interpreter was weak: a full 320 x 240 redraw takes tens of milliseconds, not 0.4 s. The animated dial runs at the firmware's own limit either way.
3. The cost is an emulator we own: 970 lines that must follow what GCC emits. A new instruction shows up as a clear stop ("undefined instruction at pc, opcode"), not as a wrong picture. A product needs an instruction test against QEMU in CI.
   Run the slice in both and compare the frame streams, as this spike did by hand.
4. To make it a product, decide between semihosting and the mapped display as the supported path.
   The unchanged firmware is the stronger claim; the mapped display is the faster and simpler one.
   Add the NVIC and PSP if an RTOS example is wanted.
   Add the FPU if a hard-float build is wanted.
   Measure in real browsers and on a phone.
   Put the emulator under a licence and into `tools/` or a `web/` folder with its tests.
5. A decoded-instruction cache would likely double the speed. It is not needed for 320 x 240.
6. Keep the QEMU route as the reference, not the delivery: it is what the small emulator is checked against.
7. The RISC-V route stays open but costs a second toolchain and a second firmware. It gives nothing the Thumb-2 interpreter does not already give.

## Run it again

```sh
cd wasm-small
emu/build.sh
node emu/run.mjs fw-qemu/examples/mcu/slice.elf --frames out-slice-m4.bin --png out-slice-m4.png
node emu/live.mjs fw-qemu/examples/mcu/live.elf 10 --press 60 197
node emu/bench.mjs fw-arm/bench.elf 320 240
python3 emu/make-page.py fw-qemu/examples/mcu/live.elf page.html   # then open page.html
```
