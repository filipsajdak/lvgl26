#!/usr/bin/env bash
# Builds GCC trunk with the third-party WebAssembly back end of seo-rii/wasm-llvm (pull request 57).
# The back end is the fork's commit ac20dcd5 minus its upstream base 33ac889a, plus 11 patches
# (producer/gcc-wasi/patches in that project). Run inside the container of tools/wasm/Dockerfile.
#   fetch-gcc.sh <work-dir> <wasm-llvm checkout>   then build-gcc.sh, build-libstdcxx.sh
set -euo pipefail
work=$1; recipe=$2/producer/gcc-wasi
fork=https://forge.sourceware.org/feedable/gcc-TEST.git
mkdir -p "$work/gw" && cd "$work/gw"
export GCC_WASI_WORK_DIR=$work/gw
git init -q gcc && cd gcc
git remote add origin "$fork"
git fetch -q --depth 1 origin ac20dcd5f8c5ae858f9b2d9cdf4140c0738e5e27
git checkout -q --detach FETCH_HEAD
python3 "$recipe/prepare-source.py" --source . --apply > ../gcc-source.json
git add -A -f . && git -c user.name=x -c user.email=x@x commit -qm wasm-all
git fetch -q --depth 1 origin 33ac889aa27d9917e6e7d07c282015b7c48e0440
git diff --binary 33ac889aa27d9917e6e7d07c282015b7c48e0440 HEAD -- . ':!gcc/testsuite' > "$work/wasm-all.patch"
# Today's trunk takes the patch with no rejects (2026-11-19, e632e263).
mkdir -p "$work/gw2" && cd "$work/gw2"
git init -q gcc && cd gcc
git fetch -q --depth 1 https://github.com/gcc-mirror/gcc.git master
git checkout -q --detach FETCH_HEAD
git apply "$work/wasm-all.patch"
