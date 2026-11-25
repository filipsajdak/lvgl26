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

A program for a device should define `handle_contract_violation` itself, as the example does. The standard library's handler brings the run time of exceptions with it: 12 KB, measured on 2026-11-15.

## A browser

`examples/wasm` is an interactive application built for WebAssembly every night, by GCC trunk with a third-party back end. It is an experiment, not a supported target.
