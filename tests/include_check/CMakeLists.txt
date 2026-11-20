# A program that takes lvgl26 by add_subdirectory, for the test include_check_build: its
# includes are checked with no line of its own (cmake/check_includes.cmake).
cmake_minimum_required(VERSION 3.30)
project(include_check LANGUAGES C CXX)
add_subdirectory(app)
add_subdirectory(${LVGL26_ROOT} lvgl26)
