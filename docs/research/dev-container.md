# Toolchain for the lvgl26-template: dev container and image options

All claims dated 2026-11-16. Marks: source, measured, summary, recalled.

## Facts

| Claim | Mark |
|---|---|
| GCC 16 enables reflection with `-std=c++26 -freflection` (PR120775); the changes page does not label it experimental | source (gcc.gnu.org/gcc-16/changes.html) |
| GCC 16.1 released 2026-04-30; P2996R13 and related papers listed | summary (isocpp.org/blog/2026/04/gcc-16.1; announcement page not read) |
| Homebrew `gcc` stable is 16.2.0, bottles for arm64_sonoma, sequoia, tahoe, golden_gate | source (formulae.brew.sh/api/formula/gcc.json) |
| Homebrew gcc 16.2 supports `-freflection` on macOS (it is the same compiler) | recalled, NOT tested; our own CLAUDE.md says GCC has no TSan on macOS |
| `gcc:16` manifest list: linux amd64, arm64/v8, arm/v5, arm/v7, ppc64le, s390x | measured (`docker buildx imagetools inspect gcc:16`) |
| `gcc:16` compressed layers: amd64 562 MB, arm64 538 MB (8 layers each) | measured (sum of layer sizes in manifest) |
| `gcc:16` uncompressed size about 1.4 GB | recalled, not measured |
| `gcc:16` lacks cmake, ninja, ccache, git as shipped (our CI Dockerfile installs them with apt) | source (.github/ci-image/Dockerfile); it is a Debian base, so git/make may partly exist: recalled |
| `ubuntu:26.04` is multi-arch incl. amd64 and arm64/v8 | measured |
| Ubuntu 26.04 (resolute) archive has gcc-16 (universe), default `gcc` stays 15 | summary (packages.ubuntu.com, Launchpad bug 2150814) |
| ubuntu-toolchain-r/test PPA offers gcc-16 on all arches incl. arm64, unsupported "test" build | summary (ubuntuhandbook.org article read; Launchpad page not read) |
| Debian has a gcc-16 package | not found |
| GitHub arm64 runners `ubuntu-24.04-arm`: GA for public repos since 2025-08, free, 4 vCPU 16 GB | summary (github.blog changelog 2025-08-07; page not read) |
| Nixpkgs has gcc16 (16.1.0 or 16.2.0, sources disagree) | summary (mynixos.com) |
| conda-forge gxx tops out at 15.2.0 in search results | summary, weak |
| A prebuilt image referenced by `image:` in devcontainer.json avoids feature build on every start; devcontainer CLI `devcontainer build --push` publishes it | summary (containers.dev/guide/prebuild) |
| Multi-arch build with `devcontainers/ci` and cacheFrom | not found |
| QEMU build time of a gcc:16 + apt layer, and pull time | not measured; estimates below are arithmetic only |

## Evaluation

1. Official `gcc:16`: both arches, about 0.55 GB compressed, GCC 16.2-based tag (tag tracks 16.x: recalled). Needs one apt layer for cmake, ninja, ccache, git, clangd. Zero toolchain maintenance. Pull at 100 Mbit/s about 45 s, at 25 Mbit/s about 3 min (arithmetic).
2. Ubuntu 26.04 + gcc-16 apt: both arches; adds a universe package, default gcc stays 15.
   Roughly same size as option 1 or smaller if only g++-16 is installed (recalled). No benefit over 1 unless an Ubuntu base is wanted. PPA on 24.04: unsupported, avoid.
3. Own multi-stage: only worth it if size matters. Building GCC from source is hours under QEMU; native `ubuntu-24.04-arm` runners avoid QEMU. Not needed while gcc:16 exists. A "FROM gcc:16 + apt" derived image built per arch on native runners and merged with `docker buildx imagetools create` is the cheap form.
4. Features vs prebuilt image: features rebuild at first start on every machine (minutes, summary). A prebuilt multi-arch image in ghcr.io makes start equal to pull time. Codespaces can use `image:` directly.
5. Alternatives: Homebrew gcc 16.2 is the best native macOS route (bottle, arm64) but reflection use is unverified here. Nix has gcc16 but adds a package manager to learn. conda-forge lacks 16. A tarball needs per-arch hosting we would maintain.

## Recommendation

Prebuilt image `ghcr.io/<org>/lvgl26-toolchain:<tag>` = the existing CI Dockerfile (FROM gcc:16 + apt), built on `ubuntu-24.04` (amd64) and `ubuntu-24.04-arm` (arm64) in a matrix, merged into one manifest list. The dev container references it by `image:`. CI and users then share one image.

## Not confirmed

- Uncompressed size and real pull time; build time of either arch.
- That `gcc:16` currently resolves to 16.2 and that `-freflection` works in the arm64 image (same source, not run).
- That our test suite builds under Docker on Apple Silicon (aarch64 GCC 16 reflection code paths).
- Homebrew gcc 16.2 on macOS building lvgl26.
- Debian gcc-16 package; Ubuntu package exact version.
- Cost and limits of `ubuntu-24.04-arm` for a private template repo (public only, per summary).
