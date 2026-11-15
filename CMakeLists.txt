cmake_minimum_required(VERSION 3.30)
project(lvgl26 VERSION 0.0.1 LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  message(FATAL_ERROR "lvgl26 needs GCC 16 or newer (C++26 reflection). Found "
                      "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
endif()

option(LVGL26_BUILD_TESTS "Build the lvgl26 tests" ${PROJECT_IS_TOP_LEVEL})
option(LVGL26_BUILD_WINDOW_EXAMPLES "Build the examples that open a window (needs SDL2)" OFF)
option(LVGL26_UPDATE_REFERENCES "Let the tests write missing reference screenshots" OFF)

include(cmake/quality.cmake)
include(cmake/lvgl.cmake)

add_library(lvgl26 INTERFACE)
add_library(lvgl26::lvgl26 ALIAS lvgl26)
target_include_directories(lvgl26 INTERFACE include)
target_compile_features(lvgl26 INTERFACE cxx_std_26)
# Designated initializers leave most members of a props struct out on purpose.
target_compile_options(lvgl26 INTERFACE -freflection -Wno-missing-field-initializers)
# lvgl26 reads LVGL's headers as text with #embed (see include/lv/detail/angle_docs.hpp).
# GCC looks for an embedded file in its embed directories, not in the include path.
target_compile_options(lvgl26 INTERFACE
  "$<LIST:TRANSFORM,$<TARGET_PROPERTY:lvgl::lvgl,INTERFACE_INCLUDE_DIRECTORIES>,PREPEND,--embed-dir=>")
# stdc++exp holds GCC's default contract violation handler.
target_link_libraries(lvgl26 INTERFACE lvgl::lvgl stdc++exp)

if(LVGL26_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(LVGL26_BUILD_WINDOW_EXAMPLES)
  add_subdirectory(examples/window)
endif()
