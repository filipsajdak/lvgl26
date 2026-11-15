# The container that the CI jobs run in: GCC 16 with the build tools already installed.
# ci.yml builds this image when this file changes and names it after the file's hash,
# so a job always runs in the image its own commit describes.
FROM gcc:16

RUN apt-get update \
 && apt-get install -y --no-install-recommends cmake ninja-build python3 python3-pip ccache git \
 && rm -rf /var/lib/apt/lists/* \
 && pip3 install --no-cache-dir --break-system-packages lizard \
 && ln -sf "$(command -v gcc)" /usr/local/bin/gcc-16 \
 && ln -sf "$(command -v g++)" /usr/local/bin/g++-16 \
 && ln -sf "$(command -v gcov)" /usr/local/bin/gcov-16
