#!/bin/sh
# Builds the slice for an Arm Cortex-M4, and the same screen as plain LVGL C, and runs both
# in QEMU. It passes if each program prints "ok". Needs tools/arm-toolchain.sh to have run, and qemu-system-arm.
#
#   tools/arm-run.sh            prints what the programs printed, and their sizes
set -eu
cd "$(dirname "$0")/.."
# GCC bug 128870 fails at random with -Os unless -fno-declone-ctor-dtor: 20 compiles of its
# reproducer must all pass (the flag is in cmake/arm/toolchain.cmake).
sysroot="${LVGL26_ARM_SYSROOT:-build/_tools/arm/arm-none-eabi}"
seq 20 | xargs -P 4 -I{} arm-none-eabi-g++ -std=gnu++26 -Os -mcpu=cortex-m4 -mthumb -fno-exceptions -fno-rtti \
    -fno-declone-ctor-dtor -isystem "${sysroot}/include/c++/16.2.0" \
    -isystem "${sysroot}/include/c++/16.2.0/arm-none-eabi" -isystem "${sysroot}/include" \
    -c tools/arm_gcc_128870.cpp -o /dev/null
cmake --preset arm-m4 "$@" > /dev/null
# The work-around must reach the compiler: the build must name the flag.
grep -q -- "-fno-declone-ctor-dtor" build/arm-m4/build.ninja
cmake --build --preset arm-m4 > /dev/null
# One run-time set must stay within its stack budget (tools/stack_budget.py).
python3 tools/stack_budget.py build/arm-m4/examples/mcu/CMakeFiles/set_stack_probe.dir/set_probe.cpp.su
# Runs one program in QEMU and prints what it printed, then its sizes.
run() {
  qemu-system-arm -M mps2-an386 -cpu cortex-m4 -nographic \
      -semihosting-config enable=on,target=native -kernel "$1" > "$2" 2>&1 &
  qemu=$!
  # The program ends by itself; the limit is for one that hangs.
  for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    kill -0 "${qemu}" 2>/dev/null || break
    sleep 1
  done
  kill "${qemu}" 2>/dev/null || true
  cat "$2"
  arm-none-eabi-size "$1"
  grep -q '^ok$' "$2"
}
elf=build/arm-m4/examples/mcu/slice.elf
run "${elf}" build/arm-m4/slice.out
# Nothing in the program can throw: the exception run-time must not be linked in.
if arm-none-eabi-nm "${elf}" | grep -E '__cxa_throw|__cxa_allocate_exception|__gxx_personality'; then
  echo "the exception run-time is linked into ${elf}" >&2
  exit 1
fi
# The same screen as plain LVGL C (tools/arm_slice_c.c): the baseline in docs/targets.md.
echo
run build/arm-m4/examples/mcu/slice_c.elf build/arm-m4/slice_c.out
