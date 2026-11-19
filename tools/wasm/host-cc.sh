#!/bin/sh
# usage: cc1.sh <src> <out> <file>
o="$2/obj/$(echo "$3" | tr / _).o"
${CC:-gcc-16} -O2 -w $(cat "$(dirname "$0")/lvgl-defs") -I"$1" -I"$1/include" -c "$1/$3" -o "$o" || echo "FAIL $3"
