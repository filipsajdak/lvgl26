FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential git ca-certificates curl bison flex cmake ninja-build python3 patch tar xz-utils gzip m4 libgmp-dev libmpfr-dev libmpc-dev libisl-dev texinfo nodejs brotli file time && rm -rf /var/lib/apt/lists/*
WORKDIR /work
