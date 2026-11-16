#!/bin/sh
# Coverage-guided fuzzing with AFL++ and its GCC plugin, for the targets that need
# reflection and so cannot be built by Clang.
#
#   fuzz/afl.sh [seconds] [target]     (default 600, runtime_document)
#
# Targets: runtime_document (corpus/document), mcp_message (corpus/mcp).
# AFL_PLUGIN and LVGL_SRC say where the plugin and LVGL's source are, if not in build/.
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
target=${2:-runtime_document}
case $target in
  runtime_document) corpus=fuzz/corpus/document ;;
  mcp_message) corpus=fuzz/corpus/mcp ;;
  *) echo "afl.sh: no target '$target'. Targets: runtime_document, mcp_message" >&2; exit 2 ;;
esac
plugin=${AFL_PLUGIN:-$PWD/build/_tools/afl/afl-gcc-pass.so}
source=${LVGL_SRC:-build/_src/lvgl-v9.6.0}
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
  -Iinclude -isystem "$source" -isystem "$source/include" -isystem "$lvgl" \
  --embed-dir="$source/include" \
  "fuzz/$target.cpp" $extra "$lvgl/liblvgl.a" "$runtime" -lstdc++exp \
  -o "$out/${target}_afl"

rm -rf "$out/afl_out_$target"
AFL_NO_UI=1 AFL_SKIP_CPUFREQ=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 AFL_NO_AFFINITY=1 \
  afl-fuzz -V "$seconds" -m none -t 2000 -i "$corpus" -o "$out/afl_out_$target" \
  -- "$out/${target}_afl" @@ > "$out/afl.log" 2>&1 || { tail -20 "$out/afl.log"; exit 1; }
grep -E "execs_done|corpus_count|edges_found|saved_crashes|saved_hangs" "$out/afl_out_$target/default/fuzzer_stats"
