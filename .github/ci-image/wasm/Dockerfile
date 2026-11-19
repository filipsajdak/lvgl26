# The container of the nightly WebAssembly check: GCC trunk with a third-party WebAssembly back
# end, libstdc++ for wasm32-wasi built by it, and the tools that link and run the result.
#
# Nothing of the compiler or of the back end is in this repository. Every input is fetched at a
# commit or against a SHA-256, below. The back end is the project seo-rii/wasm-llvm (MIT), the
# fork feedable/gcc-TEST, and a WABT fork. The compiler is GPL-3.0-or-later with its runtime
# exceptions, the WASI SDK is Apache-2.0 with LLVM exceptions, WABT is Apache-2.0. The image
# is used inside CI and is not distributed.
#
# The build context is the repository root:  docker build -f .github/ci-image/wasm/Dockerfile .
# docs/research/web-target.md says what the target is, and what it is not.
FROM ubuntu:24.04@sha256:73b999393d07f2bf65a40373778a44dc9a6d54ad7718d29247d8ff9413c4d6a8 AS build

ARG TARGETARCH
# The back end: the patches and the recipes of seo-rii/wasm-llvm, pull request 57.
ARG WASM_LLVM_COMMIT=91e8e64568949e365e0fc63674d24813b577cabf
# GCC trunk, on which the back end applies without a conflict (2026-11-19).
ARG TRUNK_COMMIT=e632e263a5ca1fb5157db28def311bbc62b8f100
# The back end and its patches, as one patch against the upstream base of the fork.
ARG PATCH_SHA256=dea4f759a5a8c3129311a7d98881ffee7106074df5a782e0202aa2ad3f901b1b
ARG WASI_SDK_SHA256_amd64=0ba8b5bfaeb2adf3f29bab5841d76cf5318ab8e1642ea195f88baba1abd47bce
ARG WASI_SDK_SHA256_arm64=4f98ee738c7abb45c81a94d1461fc53cc569d1cd01498951c8184d841a027844

RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential git ca-certificates curl bison flex cmake \
      python3 patch tar xz-utils gzip m4 libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo file \
 && rm -rf /var/lib/apt/lists/*

# The recipe: only the directory of the back end.
WORKDIR /src
RUN git init -q wasm-llvm && cd wasm-llvm \
 && git fetch -q --depth 1 https://github.com/seo-rii/wasm-llvm.git "$WASM_LLVM_COMMIT" \
 && git checkout -q --detach FETCH_HEAD \
 && cp -r producer/gcc-wasi /recipe

# The WASI SDK: wasm-ld, Clang for LVGL, the C library and the compiler runtime.
WORKDIR /work
RUN case "$TARGETARCH" in amd64) arch=x86_64; sha="$WASI_SDK_SHA256_amd64";; arm64) arch=arm64; sha="$WASI_SDK_SHA256_arm64";; esac \
 && curl -fL --retry 3 -o sdk.tgz "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-33/wasi-sdk-33.0-${arch}-linux.tar.gz" \
 && echo "${sha}  sdk.tgz" | sha256sum -c - \
 && tar -xzf sdk.tgz && rm sdk.tgz && mv wasi-sdk-33.0-${arch}-linux wasi-sdk

# The assembler for GCC's annotated WebAssembly text.
RUN GCC_WASI_WORK_DIR=/work/gw GCC_WASI_WABT_JOBS="$(nproc)" bash /recipe/build-wabt.sh native

# GCC trunk with the back end. The patch is checked against its hash.
COPY tools/wasm/ /work/lvgl26-tools/
RUN PATCH_SHA256="$PATCH_SHA256" TRUNK_COMMIT="$TRUNK_COMMIT" bash /work/lvgl26-tools/fetch-gcc.sh /work /src/wasm-llvm \
 && bash /work/lvgl26-tools/build-gcc.sh

# libstdc++ for wasm32-wasi, by that compiler, from the same tree. The recipe's own script
# is replaced by ours, which builds against trunk. Then one line of the header that WebAssembly
# cannot run (see docs/research/web-target.md).
RUN cp /work/lvgl26-tools/build-libstdcxx.sh /recipe/build-libstdcxx.sh \
 && mkdir -p /work/bin && ln -s /work/gw/wabt-native /work/gw2/wabt-native \
 && cp /work/lvgl26-tools/wasm-as /work/bin/wasm-as && cp /work/lvgl26-tools/wasm-g++ /work/bin/wg2++ \
 && GCC_WASI_WORK_DIR=/work/gw2 WASI_SDK_PATH=/work/wasi-sdk GCC_WASI_JOBS="$(nproc)" bash /recipe/build-libstdcxx.sh \
 && sh /work/lvgl26-tools/fix-coroutine.sh /work/gw2/libstdcxx/install/include/c++/17.0.0/coroutine

FROM ubuntu:24.04@sha256:73b999393d07f2bf65a40373778a44dc9a6d54ad7718d29247d8ff9413c4d6a8
ARG TARGETARCH
ARG NODE_SHA256_amd64=5c063e46bab0fbc1c17cc445d7ff129cbdcd0471be6ed83b6ab864f8143bc864
ARG NODE_SHA256_arm64=89e1853138db51e8b82d3e98ba9c0d3f2956a28e5e55ea1b7e12d25aae92d5dd
RUN apt-get update \
 && apt-get install -y --no-install-recommends git ca-certificates curl python3 xz-utils brotli \
      libgmp10 libmpfr6 libmpc3 libisl23 libatomic1 \
 && rm -rf /var/lib/apt/lists/*
# Node runs the module under WASI. A release that the nodejs.org list marks as long-term support.
RUN case "$TARGETARCH" in amd64) arch=x64; sha="$NODE_SHA256_amd64";; arm64) arch=arm64; sha="$NODE_SHA256_arm64";; esac \
 && curl -fL --retry 3 -o node.txz "https://nodejs.org/dist/v26.13.1/node-v26.13.1-linux-${arch}.tar.xz" \
 && echo "${sha}  node.txz" | sha256sum -c - \
 && tar -xJf node.txz -C /usr/local --strip-components=1 && rm node.txz
COPY --from=build /work/wasi-sdk /work/wasi-sdk
COPY --from=build /work/bin /work/bin
COPY --from=build /work/gw/wabt-native/wat2wasm /work/gw/wabt-native/wat2wasm
COPY --from=build /work/gw2/gcc-native/gcc /work/gw2/gcc-native/gcc
COPY --from=build /work/gw2/libstdcxx/install /work/gw2/libstdcxx/install
COPY --from=build /work/gw2/libstdcxx/driver /work/gw2/libstdcxx/driver
COPY --from=build /work/gw2/libstdcxx/target-headers /work/gw2/libstdcxx/target-headers
RUN ln -s /work/gw/wabt-native /work/gw2/wabt-native
# The wrapper that compiles and links for wasm32-wasi: /work/bin/wg2++
ENV PATH="/work/bin:${PATH}"
