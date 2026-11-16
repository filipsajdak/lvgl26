# The container of the nightly microcontroller check: a GCC 16.2 cross compiler for
# arm-none-eabi with newlib and libstdc++ for a Cortex-M4, and QEMU to run the result.
# Debian has no arm-none-eabi GCC 16 yet, so the compiler is built here; the libraries
# are built by the same script a developer runs (tools/arm-toolchain.sh).
# The build context is the repository root:  docker build -f .github/ci-image/arm/Dockerfile .
FROM gcc:16@sha256:5c2f83335c97e233c80d2bc754bd5c46b2d9c32f236a765a4ac40a59f216c064 AS build

RUN apt-get update \
 && apt-get install -y --no-install-recommends binutils-arm-none-eabi libgmp-dev libmpfr-dev libmpc-dev \
      curl xz-utils make texinfo \
 && rm -rf /var/lib/apt/lists/*

# The compiler and libgcc. One target, so no multilibs. The assembler and linker are
# Debian's: the compiler is told where they are, or it would run the host's `as`.
WORKDIR /work
RUN curl -fL --retry 3 -o gcc.tar.xz https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz \
 && echo "e6738e29597f733270731aa90600f37ffdc045079dfc27ec7e8192cc81085c3e  gcc.tar.xz" | sha256sum -c - \
 && tar -xJf gcc.tar.xz && rm gcc.tar.xz \
 && mkdir build-gcc && cd build-gcc \
 && ../gcc-16.2.0/configure --target=arm-none-eabi --prefix=/opt/arm \
      --enable-languages=c,c++ --without-headers --with-newlib --disable-shared --disable-threads \
      --disable-nls --disable-libssp --disable-libgomp --disable-libquadmath --disable-multilib \
      --with-cpu=cortex-m4 --with-mode=thumb \
      --with-as=/usr/bin/arm-none-eabi-as --with-ld=/usr/bin/arm-none-eabi-ld \
 && make -j"$(nproc)" all-gcc all-target-libgcc \
 && make install-gcc install-target-libgcc \
 && cd .. && rm -rf build-gcc gcc-16.2.0
ENV PATH="/opt/arm/bin:${PATH}"

# newlib and libstdc++.
COPY tools/arm-toolchain.sh /work/arm-toolchain.sh
RUN sh /work/arm-toolchain.sh /opt/arm && rm -rf /opt/arm/work

FROM gcc:16@sha256:5c2f83335c97e233c80d2bc754bd5c46b2d9c32f236a765a4ac40a59f216c064
RUN apt-get update \
 && apt-get install -y --no-install-recommends cmake ninja-build python3 git binutils-arm-none-eabi \
      qemu-system-arm libgmp10 libmpfr6 libmpc3 \
 && rm -rf /var/lib/apt/lists/*
COPY --from=build /opt/arm /opt/arm
ENV PATH="/opt/arm/bin:${PATH}"
# Where cmake/arm/toolchain.cmake finds newlib and libstdc++.
ENV LVGL26_ARM_SYSROOT=/opt/arm/arm-none-eabi
