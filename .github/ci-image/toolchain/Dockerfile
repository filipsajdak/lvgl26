# The toolchain for a project that uses lvgl26: GCC 16.2, CMake, Ninja, ccache and git on
# Debian sid, for amd64 and arm64. It is the image of the template repository and of its
# dev container. toolchain-image.yml builds it for both architectures.
#
# The base is pinned by digest, and apt reads the Debian snapshot of the same moment, so
# a rebuild gives the same packages. To move on, take the digest and the date from a new
# `debian:sid-slim` (its first lines in /etc/apt/sources.list.d/debian.sources).
FROM debian:sid-slim@sha256:07a3205e8583fdff12d2b05bd030f50437e9fd340ee616df0c05dc77af75767b

# Links the package on ghcr.io to this repository, so that the workflow may push to it.
LABEL org.opencontainers.image.source="https://github.com/filipsajdak/lvgl26" \
      org.opencontainers.image.description="GCC 16.2, CMake, Ninja, ccache and git for projects that use lvgl26"

ARG SNAPSHOT=20261104T212339Z

# g++-16 is the only compiler. Debian names it g++-16, so `g++` and `gcc` point to it.
RUN sed -i "s#http://deb.debian.org/debian#http://snapshot.debian.org/archive/debian/${SNAPSHOT}#" \
      /etc/apt/sources.list.d/debian.sources \
 && apt-get -o Acquire::Check-Valid-Until=false update \
 && apt-get install -y --no-install-recommends g++-16 libc6-dev make cmake ninja-build ccache git ca-certificates python3 \
 && rm -rf /var/lib/apt/lists/* /usr/share/doc /usr/share/man /usr/share/info /usr/share/locale \
 && ln -sf /usr/bin/g++-16 /usr/local/bin/g++ && ln -sf /usr/bin/gcc-16 /usr/local/bin/gcc \
 && ln -sf /usr/bin/g++-16 /usr/local/bin/c++ && ln -sf /usr/bin/gcc-16 /usr/local/bin/cc \
 && g++ --version | head -n 1 && cmake --version | head -n 1 && ninja --version
