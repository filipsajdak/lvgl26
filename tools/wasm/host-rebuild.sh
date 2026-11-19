#!/bin/sh
# Rebuilds only the demo program against the objects of host-demo.sh.
set -eu
src=$1; out=$2; repo=$3; here=$(cd "$(dirname "$0")" && pwd)
python3 "$here/make-script.py" "$repo/examples/wasm/script.json" > "$out/script.inc"
g++-16 -std=c++26 -freflection -fno-exceptions -fno-rtti -O2 -Wno-missing-field-initializers $(cat "$here/lvgl-defs") \
  -I"$repo/include" -isystem "$src" -isystem "$src/include" --embed-dir="$src" --embed-dir="$src/include" \
  -I"$out" -I"$repo/examples/wasm" "$repo/examples/wasm/platform.cpp" "$repo/examples/wasm/host_main.cpp" "$out"/obj/*.o -o "$out/demo-host"
