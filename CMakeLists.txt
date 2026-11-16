cmake_minimum_required(VERSION 3.30)
project(lvgl26 VERSION 0.0.1 LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  message(FATAL_ERROR "lvgl26 needs GCC 16 or newer (C++26 reflection). Found "
                      "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
endif()

option(LVGL26_BUILD_TESTS "Build the lvgl26 tests" ${PROJECT_IS_TOP_LEVEL})
option(LVGL26_BUILD_MCU_EXAMPLE "Build the slice for a microcontroller (preset arm-m4)" OFF)
option(LVGL26_PNG "Build LVGL with its PNG decoder, for PNG files decoded on the device (lv::encoded_image)" OFF)
option(LVGL26_JPEG "Build LVGL with its JPEG decoder, for JPEG files decoded on the device (lv::encoded_image)" OFF)
option(LVGL26_TTF "Build LVGL with TinyTTF, for fonts drawn on the device (lv::scaled_font)" OFF)
option(LVGL26_UNITS "Fetch mp-units and link it to lvgl26, for values with a unit" OFF)
option(LVGL26_UPDATE_REFERENCES "Let the tests write missing reference screenshots" OFF)

# lvgl26 has no C++ modules. CMake would still scan every C++ source for them and pass
# -fmodules-ts with a module mapper to the compiler, and ccache refuses to cache such a
# call. A target that gets modules turns the scan on for itself with its CXX_SCAN_FOR_MODULES
# property.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(cmake/quality.cmake)
include(cmake/lvgl.cmake)
include(cmake/editor.cmake)

add_library(lvgl26 INTERFACE)
add_library(lvgl26::lvgl26 ALIAS lvgl26)
target_include_directories(lvgl26 INTERFACE include)
target_compile_features(lvgl26 INTERFACE cxx_std_26)
# Designated initializers leave most members of a props struct out on purpose.
target_compile_options(lvgl26 INTERFACE -freflection -Wno-missing-field-initializers)
# The compiler draws fonts (include/lv/font.hpp). GCC stops a constant evaluation after 33
# million operations by default; a 48 px font of 95 characters needs about 100 million.
target_compile_options(lvgl26 INTERFACE -fconstexpr-ops-limit=1000000000)
# lvgl26 reads LVGL's headers as text with #embed (see include/lv/detail/angle_docs.hpp).
# GCC looks for an embedded file in its embed directories, not in the include path.
target_compile_options(lvgl26 INTERFACE
  "$<LIST:TRANSFORM,$<TARGET_PROPERTY:lvgl::lvgl,INTERFACE_INCLUDE_DIRECTORIES>,PREPEND,--embed-dir=>")
# stdc++exp holds GCC's default contract violation handler.
target_link_libraries(lvgl26 INTERFACE lvgl::lvgl stdc++exp)
if(LVGL26_UNITS)
  target_link_libraries(lvgl26 INTERFACE mp-units::mp-units)
endif()

if(LVGL26_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(LVGL26_BUILD_MCU_EXAMPLE)
  add_subdirectory(examples/mcu)
endif()
