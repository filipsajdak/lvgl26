#!/bin/sh
# Builds the libraries that a bare arm-none-eabi GCC 16 lacks: a C library (newlib) and
# the C++ standard library (libstdc++ from the GCC source), for one Cortex-M target.
#
#   tools/arm-toolchain.sh <prefix> [cpu]        default cpu: cortex-m4
#
# Needs arm-none-eabi-gcc 16.2.0 and arm-none-eabi-binutils on PATH (Homebrew has both),
# make, curl and about 1.5 GB of disk. The result is in <prefix>/arm-none-eabi.
# Use it with:  arm-none-eabi-g++ --sysroot=<prefix>/arm-none-eabi ... (see cmake/arm.cmake)
set -eu

prefix="$(mkdir -p "$1" && cd "$1" && pwd)"
cpu="${2:-cortex-m4}"
gcc_version="16.2.0"
newlib_version="4.5.0.20241231"
newlib_sha256="33f12605e0054965996c25c1382b3e463b0af91799001f5bb8c0630f2ec8c852"
gcc_sha256="e6738e29597f733270731aa90600f37ffdc045079dfc27ec7e8192cc81085c3e"  # gcc-16.2.0.tar.xz
target_flags="-mcpu=${cpu} -mthumb -ffunction-sections -fdata-sections -Os"
work="${prefix}/work"
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

have="$(arm-none-eabi-gcc -dumpfullversion)"
[ "${have}" = "${gcc_version}" ] || { echo "arm-none-eabi-gcc is ${have}, this script builds libstdc++ of ${gcc_version}" >&2; exit 1; }

mkdir -p "${work}"
cd "${work}"

fetch() {  # fetch <url> <file>
  [ -f "$2" ] || { curl -fL --retry 3 -o "$2.part" "$1" && mv "$2.part" "$2"; }
}

# ---- newlib: the C library ---------------------------------------------------
if [ ! -f "${prefix}/arm-none-eabi/lib/libc.a" ]; then
  fetch "https://sourceware.org/pub/newlib/newlib-${newlib_version}.tar.gz" newlib.tar.gz
  echo "${newlib_sha256}  newlib.tar.gz" | sha256sum -c -
  rm -rf "newlib-${newlib_version}" build-newlib
  tar -xzf newlib.tar.gz
  mkdir build-newlib
  ( cd build-newlib
    "../newlib-${newlib_version}/configure" --target=arm-none-eabi --prefix="${prefix}" \
        --disable-multilib --disable-nls --disable-newlib-supplied-syscalls \
        --enable-newlib-reent-small --enable-newlib-nano-malloc --enable-newlib-nano-formatted-io \
        --disable-newlib-fvwrite-in-streamio --disable-newlib-wide-orient --enable-lite-exit \
        CFLAGS_FOR_TARGET="${target_flags}"
    # MAKEINFO=true: the manuals are not needed, and makeinfo is often not installed.
    make -j"${jobs}" MAKEINFO=true
    make install MAKEINFO=true )
fi

# ---- libstdc++: the C++ standard library --------------------------------------
if [ ! -f "${prefix}/arm-none-eabi/lib/libstdc++.a" ]; then
  fetch "https://ftp.gnu.org/gnu/gcc/gcc-${gcc_version}/gcc-${gcc_version}.tar.xz" gcc.tar.xz
  echo "${gcc_sha256}  gcc.tar.xz" | sha256sum -c -
  rm -rf "gcc-${gcc_version}" build-libstdcxx
  # Only what libstdc++ needs of the GCC source: the whole tree is about 1 GB.
  tar -xJf gcc.tar.xz "gcc-${gcc_version}/libstdc++-v3" "gcc-${gcc_version}/libgcc" \
      "gcc-${gcc_version}/config" "gcc-${gcc_version}/include" "gcc-${gcc_version}/libiberty" \
      "gcc-${gcc_version}/gcc/BASE-VER" "gcc-${gcc_version}/gcc/DATESTAMP" \
      "gcc-${gcc_version}/config.sub" "gcc-${gcc_version}/config.guess" "gcc-${gcc_version}/install-sh" \
      "gcc-${gcc_version}/ltmain.sh" "gcc-${gcc_version}/missing" "gcc-${gcc_version}/mkinstalldirs" \
      "gcc-${gcc_version}/libtool.m4" "gcc-${gcc_version}/config-ml.in" "gcc-${gcc_version}/multilib.am" \
      "gcc-${gcc_version}/move-if-change" "gcc-${gcc_version}/depcomp" "gcc-${gcc_version}/compile" \
      "gcc-${gcc_version}/ar-lib" "gcc-${gcc_version}/test-driver" "gcc-${gcc_version}/ylwrap" \
      "gcc-${gcc_version}/libtool-ldflags" "gcc-${gcc_version}/contrib/relpath.sh" \
      "gcc-${gcc_version}/libbacktrace" 2>/dev/null || true
  # configure links small programs to find out what the C library has. A bare compiler
  # cannot link, so it is pointed at newlib's start-up file and its empty system calls
  # (-B and nosys.specs below).
  # The library itself is built with exceptions and run-time type information: its own
  # sources need them. A program that uses it can still turn both off.
  mkdir build-libstdcxx
  ( cd build-libstdcxx
    sysroot="${prefix}/arm-none-eabi"
    CC="arm-none-eabi-gcc" CXX="arm-none-eabi-g++" AR="arm-none-eabi-ar" RANLIB="arm-none-eabi-ranlib" \
    CPPFLAGS="-isystem ${sysroot}/include" \
    CFLAGS="${target_flags} -isystem ${sysroot}/include -B${sysroot}/lib/" \
    CXXFLAGS="${target_flags} -isystem ${sysroot}/include -B${sysroot}/lib/" \
    LDFLAGS="-L${sysroot}/lib --specs=nosys.specs" \
    "../gcc-${gcc_version}/libstdc++-v3/configure" --host=arm-none-eabi --prefix="${prefix}" \
        --with-newlib --disable-multilib --disable-nls --disable-shared --disable-libstdcxx-pch \
        --disable-libstdcxx-verbose --disable-libstdcxx-threads --disable-libstdcxx-filesystem-ts \
        --disable-libstdcxx-backtrace --with-gxx-include-dir="${sysroot}/include/c++/${gcc_version}" \
        --libdir="${sysroot}/lib"
    make -j"${jobs}"
    make install )
fi

echo "done: ${prefix}/arm-none-eabi"
