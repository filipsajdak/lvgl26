#!/usr/bin/env bash
set -euxo pipefail
work=/work/gw2; sdk=/work/wasi-sdk-33.0-arm64-linux
mkdir -p $work/gcc-native && cd $work/gcc-native
$work/gcc/configure --target=wasm32-wasi --prefix=$work/native-install \
  --with-sysroot=$sdk/share/wasi-sysroot --enable-languages=c,c++ \
  --disable-bootstrap --disable-analyzer --disable-multilib --disable-nls \
  --disable-shared --disable-threads --disable-libstdcxx --disable-libsanitizer \
  --disable-libssp --disable-libquadmath --disable-libgomp --disable-libatomic \
  --disable-libvtv --disable-libcc1 --disable-libitm --disable-lto --disable-plugin \
  --disable-werror --enable-checking=release --disable-fixincludes --without-headers \
  CFLAGS='-O1 -g0' CXXFLAGS='-O1 -g0' MAKEINFO=true
/usr/bin/time -v make -j6 all-gcc MAKEINFO=true || true
make -j6 -C gcc cc1 cc1plus MAKEINFO=true
make -C gcc stmp-int-hdrs STMP_FIXINC= LIMITS_H_TEST=true MAKEINFO=true
test -x gcc/cc1plus && gcc/cc1plus --version | head -1
echo BUILD3-DONE
