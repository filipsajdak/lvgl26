#!/bin/sh
# Builds the slice for an Arm Cortex-M4 and runs it in QEMU. It passes if the program
# prints "ok". Needs tools/arm-toolchain.sh to have run, and qemu-system-arm.
#
#   tools/arm-run.sh            prints what the program printed, and its sizes
set -eu
cd "$(dirname "$0")/.."
cmake --preset arm-m4 "$@" > /dev/null
cmake --build --preset arm-m4 > /dev/null
# One run-time set must stay within its stack budget (tools/stack_budget.py).
python3 tools/stack_budget.py build/arm-m4/examples/mcu/CMakeFiles/set_stack_probe.dir/set_probe.cpp.su
elf=build/arm-m4/examples/mcu/slice.elf
out=build/arm-m4/slice.out
qemu-system-arm -M mps2-an386 -cpu cortex-m4 -nographic \
    -semihosting-config enable=on,target=native -kernel "${elf}" > "${out}" 2>&1 &
qemu=$!
# The program ends by itself; the limit is for one that hangs.
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  kill -0 "${qemu}" 2>/dev/null || break
  sleep 1
done
kill "${qemu}" 2>/dev/null || true
cat "${out}"
arm-none-eabi-size "${elf}"
grep -q '^ok$' "${out}"
# Nothing in the program can throw: the exception run-time must not be linked in.
if arm-none-eabi-nm "${elf}" | grep -E '__cxa_throw|__cxa_allocate_exception|__gxx_personality'; then
  echo "the exception run-time is linked into ${elf}" >&2
  exit 1
fi
