# The container of the job that reads the editor stubs with Clang (ci.yml, job editor).
# It holds what configuring the project needs, as the main image does, and on top of it
# clang++ and clangd: the eight editor_* tests that need them run nowhere else in CI.
#
# Clang 21 comes from apt.llvm.org. Debian 13, which gcc:16 is made from, has Clang 19,
# and Clang 19 does not compile mp-units 2.5.0, which the quick start includes. Clang
# reads the headers of Debian's libstdc++ 14, not those of GCC 16 in /usr/local.
#
# The main image stays without Clang: it is several hundred megabytes that only this
# job uses.
FROM gcc:16

ARG CLANG=21
RUN apt-get update \
 && apt-get install -y --no-install-recommends cmake ninja-build python3 ccache git curl ca-certificates gnupg \
 && curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
 && echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${CLANG} main" \
      > /etc/apt/sources.list.d/llvm.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends clang-${CLANG} clangd-${CLANG} \
 && rm -rf /var/lib/apt/lists/* \
 && ln -sf "$(command -v gcc)" /usr/local/bin/gcc-16 \
 && ln -sf "$(command -v g++)" /usr/local/bin/g++-16 \
 && ln -sf "$(command -v clang++-${CLANG})" /usr/local/bin/clang++ \
 && ln -sf "$(command -v clangd-${CLANG})" /usr/local/bin/clangd \
 && clang++ --version && clangd --version
