# The container that the CI jobs run in: GCC 16 with the build tools already installed.
# ci.yml builds this image when this file changes and names it after the file's hash,
# so a job always runs in the image its own commit describes.
FROM gcc:16@sha256:5c2f83335c97e233c80d2bc754bd5c46b2d9c32f236a765a4ac40a59f216c064

RUN apt-get update \
 && apt-get install -y --no-install-recommends cmake ninja-build python3 python3-pip ccache git rsync valgrind=1:3.24.* libc6-dbg \
 && rm -rf /var/lib/apt/lists/* \
 && pip3 install --no-cache-dir --break-system-packages lizard==1.26.1 universalmutator==1.14.1 jsonschema==4.26.0 \
 && ln -sf "$(command -v gcc)" /usr/local/bin/gcc-16 \
 && ln -sf "$(command -v g++)" /usr/local/bin/g++-16 \
 && ln -sf "$(command -v gcov)" /usr/local/bin/gcov-16
