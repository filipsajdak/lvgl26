#!/bin/sh
# Builds the demo for the host (GCC 16) with LVGL built by the same compiler.
#   host-demo.sh <lvgl-src> <out-dir> <repo> <app>   writes <out-dir>/demo-host; <app> is counter or bounce
set -eu
src=$1; out=$2; repo=$3; app=${4:-counter}
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$out/obj"
if [ ! -f "$out/obj/.done" ]; then
  (cd "$src" && find src -name '*.c' | sort > "$out/files.txt")
  xargs -P6 -n1 "$here/host-cc.sh" "$src" "$out" < "$out/files.txt"
  touch "$out/obj/.done"
fi
python3 "$here/make-script.py" "$repo/examples/wasm/$app/script.json" > "$out/script.inc"
${CXX:-g++-16} -std=c++26 -freflection -fno-exceptions -fno-rtti -O2 -Wno-missing-field-initializers $(cat "$here/lvgl-defs") \
  -I"$repo/include" -isystem "$src" -isystem "$src/include" --embed-dir="$src" --embed-dir="$src/include" \
  -I"$out" -I"$repo/examples/wasm/$app" -I"$repo/examples/wasm" --embed-dir="$repo/examples/wasm/$app" "$repo/examples/wasm/platform.cpp" "$repo/examples/wasm/host_main.cpp" "$out"/obj/*.o -o "$out/demo-host"
