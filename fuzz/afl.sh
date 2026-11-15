#!/bin/sh
# Coverage-guided fuzzing of the document loader with AFL++ and its GCC plugin.
#
#   fuzz/afl.sh [seconds]        (default 600)
#
# Needs: the host-v96-asan preset built (for LVGL), afl-fuzz on the PATH, and the
# plugin afl-gcc-pass.so in build/_tools/afl/. Homebrew's AFL++ has no GCC plugin;
# build it from the AFL++ source of the same version:
#   CC=gcc-16 CXX=g++-16 CPPFLAGS=-I/opt/homebrew/include \
#     CXXFLAGS="-I/opt/homebrew/include -Wl,-undefined,dynamic_lookup" \
#     make -f GNUmakefile.gcc_plugin LDFLAGS= afl-gcc-pass.so
# AFL_RT is AFL++'s runtime object; the default is where Homebrew puts it.
set -eu
cd "$(dirname "$0")/.."
seconds=${1:-600}
plugin=$PWD/build/_tools/afl/afl-gcc-pass.so
runtime=${AFL_RT:-/opt/homebrew/lib/afl/afl-compiler-rt.o}
lvgl=build/host-v96-asan/_deps/lvgl-build
out=build/fuzz
mkdir -p "$out"

# GCC on macOS emulates thread-local storage; see fuzz/afl_prev_loc.c.
extra=
if [ "$(uname)" = Darwin ]; then
  gcc-16 -c fuzz/afl_prev_loc.c -o "$out/afl_prev_loc.o"
  extra=$out/afl_prev_loc.o
fi

g++-16 -std=gnu++26 -freflection -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
  -fplugin="$plugin" \
  -DLV_CONF_SKIP -DLV_LVGL_H_INCLUDE_SIMPLE -DLV_MEM_SIZE=4194304 -DLV_USE_TEST=1 \
  -DLV_USE_SNAPSHOT=1 -DLV_USE_LODEPNG=1 -DLV_USE_FS_STDIO=1 -DLV_FS_STDIO_LETTER=65 \
  -DLV_USE_TEST_SCREENSHOT_COMPARE=1 \
  -Iinclude -isystem build/_src/lvgl-v9.6.0 -isystem build/_src/lvgl-v9.6.0/include -isystem "$lvgl" \
  fuzz/runtime_document.cpp $extra "$lvgl/liblvgl.a" "$runtime" -lstdc++exp \
  -o "$out/runtime_document_afl"

rm -rf "$out/afl_out"
AFL_NO_UI=1 AFL_SKIP_CPUFREQ=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 AFL_NO_AFFINITY=1 \
  afl-fuzz -V "$seconds" -m none -t 2000 -i fuzz/corpus/document -o "$out/afl_out" \
  -- "$out/runtime_document_afl" @@ > "$out/afl.log" 2>&1 || { tail -20 "$out/afl.log"; exit 1; }
grep -E "execs_done|corpus_count|edges_found|saved_crashes|saved_hangs" "$out/afl_out/default/fuzzer_stats"
