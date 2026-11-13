cmake_minimum_required(VERSION 3.30)
project(lvgl26 VERSION 0.0.1 LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  message(FATAL_ERROR "lvgl26 needs GCC 16 or newer (C++26 reflection). Found "
                      "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
endif()

option(LVGL26_BUILD_TESTS "Build the lvgl26 tests" ${PROJECT_IS_TOP_LEVEL})
option(LVGL26_UPDATE_REFERENCES "Let the tests write missing reference screenshots" OFF)

include(cmake/lvgl.cmake)

add_library(lvgl26 INTERFACE)
add_library(lvgl26::lvgl26 ALIAS lvgl26)
target_include_directories(lvgl26 INTERFACE include)
target_compile_features(lvgl26 INTERFACE cxx_std_26)
target_compile_options(lvgl26 INTERFACE -freflection)
# stdc++exp holds GCC's default contract violation handler.
target_link_libraries(lvgl26 INTERFACE lvgl::lvgl stdc++exp)

if(LVGL26_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
