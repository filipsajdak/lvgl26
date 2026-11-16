# The quick start of the documentation, as a project of its own: copy this folder
# and it builds. docs/quick-start.md walks through it.
cmake_minimum_required(VERSION 3.30)
project(quick_start LANGUAGES C CXX)

include(FetchContent)

# LVGL's own heap: a picture of the whole screen needs more than LVGL's 64 KB.
set(LVGL26_LVGL_HEAP_BYTES 1048576)

# lvgl26 brings the LVGL it is pinned to. Pin a commit of lvgl26 for a product.
FetchContent_Declare(lvgl26
  GIT_REPOSITORY https://github.com/filipsajdak/lvgl26
  GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

# mp-units gives the speed its unit. lvgl26 does not include it: you choose it.
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)
set(MP_UNITS_API_STD_FORMAT TRUE CACHE STRING "" FORCE)
set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(mp-units
  URL https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz
  SOURCE_SUBDIR src
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(mp-units)

add_executable(quick_start main.cpp)
target_link_libraries(quick_start PRIVATE lvgl26::lvgl26 mp-units::mp-units)
