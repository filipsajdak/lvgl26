# Targets

Where a program of lvgl26 runs: a desktop, a microcontroller, and, as an experiment, a browser.

Header: none of its own.

## A desktop

A window on the desktop is a platform object that feeds the display and the pointer. `examples/window` opens one with SDL: [A pointer](input.md).

## A microcontroller

The library builds with `-fno-exceptions -fno-rtti`. `examples/mcu/slice.cpp` runs on an Arm Cortex-M4 in QEMU every night: a compile-time tree, a label bound to a state, and a task.

```sh
brew install arm-none-eabi-binutils arm-none-eabi-gcc qemu   # GCC 16.2, without its libraries
tools/arm-toolchain.sh build/_tools/arm                      # builds newlib and libstdc++, once
tools/arm-run.sh                                             # builds the slice, runs it in QEMU
```

The nightly run of 2026-11-21 measured this, with LVGL's default configuration, where every widget and feature is on:

| What | Size |
|---|---|
| Code of the slice (text) | 279,188 bytes |
| LVGL's heap for the screen | 988 bytes |
| LVGL's heap for the task | 128 bytes |

On 2026-11-15 the application and the display objects were 1.6 KB of the code.

### The same screen in plain C

`tools/arm_slice_c.c` makes the screen of the slice with LVGL's C API alone: the same widgets, the label bound to an LVGL subject, and an LVGL timer that runs three times where the slice runs a task. The preset `arm-m4` builds it with the same LVGL, the same configuration and the same flags, and `tools/arm-run.sh` runs it after the slice. Both columns come from one local build of 2026-11-28 (commit 34a61ac), not from the nightly run above.

<!-- arm-c-baseline: both columns from one build; the nightly table above is a separate measurement -->
| What | lvgl26 (`slice.elf`) | Plain C (`slice_c.elf`) |
|---|---|---|
| `.text` | 279,500 bytes | 244,288 bytes |
| `.data` | 4,224 bytes | 184 bytes |
| `.bss` | 73,016 bytes | 73,020 bytes |
| LVGL's heap for the screen | 988 bytes | 964 bytes |
| LVGL's heap for the task (C: the timer) | 128 bytes | 36 bytes |

What differs, from the object file of the slice and the two link maps:

- The slice's own object file holds 17,156 bytes of strings: the names of the functions whose contracts it checks. Its `.data` holds the records of those 112 contract checks. The C program checks none.
- One run-time `set` of the dial is 6,300 bytes of code; the C program calls one setter.
- LVGL's library adds 4.5 KB more code to the slice than to the C program: functions the slice links and the C program does not.
- The slice follows its state with lvgl26's own watcher list; the C program uses LVGL's subject. The slice's display puts a theme of lvgl26 over LVGL's default theme; the C program has the default theme only.
- The slice's task is a coroutine frame in LVGL's heap that waits on LVGL timers; the C task is one timer.

A program for a device should define `handle_contract_violation` itself, as the example does. The standard library's handler brings the run time of exceptions with it: 12 KB, measured on 2026-11-15.

## A browser

`examples/wasm` is an interactive application built for WebAssembly every night, by GCC trunk with a third-party back end. It is an experiment, not a supported target.
