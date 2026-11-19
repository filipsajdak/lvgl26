#!/usr/bin/env bash
# Builds LVGL v9.6.0 for wasm32-wasi with the WASI SDK's Clang. Usage: lvgl-build.sh <lvgl-src> <out-dir> [extra cflags]
set -euo pipefail
src=$1; out=$2; shift 2
sdk=${WASI_SDK_PATH:-/work/wasi-sdk-33.0-arm64-linux}
mkdir -p "$out/obj"
cd "$src"
find src -name '*.c' | sort > "$out/files.txt"
compile() {
  f=$1; o="$OUT/obj/$(echo "$f" | tr '/' '_').o"
  "$SDK/bin/clang" --target=wasm32-wasi --sysroot="$SDK/share/wasi-sysroot" -Os -ffunction-sections -fdata-sections \
    -DLV_CONF_SKIP=1 -DLV_USE_SNAPSHOT=1 $EXTRA -I"$SRC" -I"$SRC/include" -c "$f" -o "$o"
}
export -f compile; export OUT="$out" SDK="$sdk" SRC="$src" EXTRA="$*"
xargs -P6 -I{} bash -c 'compile {}' < "$out/files.txt"
"$sdk/bin/llvm-ar" rcs "$out/liblvgl.a" "$out"/obj/*.o
ls -la "$out/liblvgl.a"
