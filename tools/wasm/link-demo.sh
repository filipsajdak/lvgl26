#!/bin/sh
# Compiles and links the demo as a reactor module with exports (run in the container).
#   link-demo.sh <repo> <liblvgl.a dir> <out.wasm> [extra compile flags]
set -eu
repo=$1; lib=$2; out=$3; shift 3
here=$(cd "$(dirname "$0")" && pwd)
sysroot=/work/wasi-sdk-33.0-arm64-linux/share/wasi-sysroot/lib/wasm32-wasi
flags="$(cat "$here/flags" | sed 's/-DLV_CONF_SKIP=1 -DLV_USE_SNAPSHOT=1//') $(cat "$here/lvgl-defs")"
/work/bin/wg2++ $flags -O2 "$@" -I"$repo/examples/wasm" -c "$repo/examples/wasm/platform.cpp" -o /tmp/platform-wasm.o
/work/bin/wg2++ -nostartfiles "$sysroot/crt1-reactor.o" /tmp/platform-wasm.o "$lib/liblvgl.a" -o "$out" \
  -Wl,--gc-sections -Wl,--no-entry -Wl,--strip-all \
  -Wl,--export=_initialize -Wl,--export=web_init -Wl,--export=web_step -Wl,--export=web_frame \
  -Wl,--export=web_stride -Wl,--export=web_width -Wl,--export=web_height -Wl,--export=web_take_dirty -Wl,--export=web_dirty \
  -Wl,--export=web_heap_used -lstdc++ -lsupc++
