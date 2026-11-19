#!/bin/sh
# LVGL v9.6.0, checked against the hash that cmake/lvgl.cmake pins.   fetch-lvgl.sh <dir>
set -eu
dir=$1
mkdir -p "$dir"
curl -fsSL --retry 3 -o "$dir/lvgl.tar.gz" https://github.com/lvgl/lvgl/archive/v9.6.0.tar.gz
echo "b20ee3acc1bba13c62d854f9ebd62e4c51e0b443b1e0225892e86442defa84df  $dir/lvgl.tar.gz" | sha256sum -c -
mkdir -p "$dir/src"
tar -xzf "$dir/lvgl.tar.gz" -C "$dir/src" --strip-components=1
