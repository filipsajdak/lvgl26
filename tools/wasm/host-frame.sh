#!/bin/sh
# usage: host-frame.sh <lvgl-src> <out-dir> <frame.cpp> <lvgl26-include>
set -eu
src=$1; out=$2; cpp=$3; inc=$4
mkdir -p "$out/obj"
cd "$src"
find src -name "*.c" | sort > "$out/files.txt"
xargs -P6 -n1 /Users/filipsajdak/.claude-scudo/jobs/4d35e9c8/tmp/run/cc1.sh "$src" "$out" < "$out/files.txt"
g++-16 -std=c++26 -freflection -fno-exceptions -fno-rtti -O2 -Wno-missing-field-initializers -DLV_CONF_SKIP=1 -DLV_USE_SNAPSHOT=1 \
  -I"$inc" -isystem "$src" -isystem "$src/include" --embed-dir="$src" --embed-dir="$src/include" \
  "$cpp" "$out"/obj/*.o -o "$out/frame-host"
