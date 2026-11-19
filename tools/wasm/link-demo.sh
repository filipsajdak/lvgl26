#!/bin/sh
# Compiles the demo with the WebAssembly GCC and links it as a reactor module with exports.
# Runs where tools/wasm/Dockerfile's image runs.
#   link-demo.sh <repo> <lvgl-src> <dir of liblvgl.a> <out.wasm> [counter|bounce] [extra compile flags]
set -eu
repo=$1; lvgl=$2; lib=$3; out=$4; app=${5:-counter}; shift 4; [ $# -gt 0 ] && shift
here=$(cd "$(dirname "$0")" && pwd)
sysroot=/work/wasi-sdk/share/wasi-sysroot/lib/wasm32-wasi
/work/bin/wg2++ -std=c++26 -freflection -fno-exceptions -fno-rtti -Wno-missing-field-initializers -O2 \
  $(cat "$here/lvgl-defs") -I"$repo/include" -isystem "$lvgl" -isystem "$lvgl/include" \
  --embed-dir="$lvgl" --embed-dir="$lvgl/include" -I"$repo/examples/wasm/$app" -I"$repo/examples/wasm" --embed-dir="$repo/examples/wasm/$app" "$@" \
  -c "$repo/examples/wasm/platform.cpp" -o "$out.o"
/work/bin/wg2++ -nostartfiles "$sysroot/crt1-reactor.o" "$out.o" "$lib/liblvgl.a" -o "$out" \
  -Wl,--gc-sections -Wl,--no-entry -Wl,--strip-all -Wl,-z,stack-size=1048576 \
  -Wl,--export=_initialize -Wl,--export=web_init -Wl,--export=web_step -Wl,--export=web_pointer -Wl,--export=web_frame \
  -Wl,--export=web_stride -Wl,--export=web_width -Wl,--export=web_height -Wl,--export=web_take_dirty \
  -Wl,--export=web_dirty -Wl,--export=web_heap_used -Wl,--export=web_state -lstdc++ -lsupc++
