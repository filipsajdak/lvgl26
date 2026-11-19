#!/bin/sh
# libstdc++'s std::noop_coroutine frame holds void(*)() pointers that the coroutine machinery calls
# as void(*)(void*). WebAssembly checks the signature of an indirect call and traps. Give the
# function the parameter it is called with. Usage: fix-coroutine.sh <installed c++ include dir>/coroutine
set -eu
sed -i 's/__dummy_resume_destroy() { }/__dummy_resume_destroy(void*) { }/; s/void (\*__r)() = /void (*__r)(void*) = /; s/void (\*__d)() = /void (*__d)(void*) = /' "$1"
