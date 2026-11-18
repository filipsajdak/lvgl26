# Warnings and sanitizers for lvgl26's own code. LVGL itself is a system header
# for these purposes: its warnings are upstream's business.

add_library(lvgl26_warnings INTERFACE)
target_compile_options(lvgl26_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Werror
  -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wcast-qual
  -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
  -Wformat=2 -Wimplicit-fallthrough -Wduplicated-cond -Wduplicated-branches
  -Wlogical-op -Wuseless-cast)

# LVGL26_SANITIZE: empty, or a -fsanitize= list such as "address,undefined" or "thread".
set(LVGL26_SANITIZE "" CACHE STRING "Sanitizers for LVGL and the tests")
if(LVGL26_SANITIZE)
  add_compile_options(-fsanitize=${LVGL26_SANITIZE} -fno-omit-frame-pointer
                      -fno-sanitize-recover=all)
  add_link_options(-fsanitize=${LVGL26_SANITIZE})
endif()

option(LVGL26_ANALYZE "Run GCC's static analyzer (-fanalyzer) on the tests" OFF)
# Under the analyzer a test file needs 1.0 to 2.4 GB at its peak (state.cpp 2.2 GB, watchers.cpp
# 2.2 GB, computed.cpp 1.9 GB, measured 2026-11-18). Four at once took a CI runner of 16 GB down
# three times ("the hosted runner lost communication"). Three at once leave room.
set(LVGL26_ANALYZE_JOBS 3 CACHE STRING "How many files the analyzer build compiles at once")
if(LVGL26_ANALYZE)
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS analyze=${LVGL26_ANALYZE_JOBS})
  set(CMAKE_JOB_POOL_COMPILE analyze)
  # A target with the property LVGL26_NO_ANALYZER is left out: see lvgl26_skip_analyzer below.
  target_compile_options(lvgl26_warnings INTERFACE
                         $<$<NOT:$<BOOL:$<TARGET_PROPERTY:LVGL26_NO_ANALYZER>>>:-fanalyzer>)
endif()

# The analyzer triples the compile time of a port (16 files of two ports: 657 s without it, 2224 s
# with it, measured 2026-11-17), and a port is the same headers again. The tests of the library
# keep it. Call this after the targets of a directory exist.
function(lvgl26_skip_analyzer directory)
  get_property(targets DIRECTORY ${directory} PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    set_property(TARGET ${target} PROPERTY LVGL26_NO_ANALYZER ON)
  endforeach()
endfunction()

option(LVGL26_COVERAGE "Build the tests with coverage instrumentation" OFF)
if(LVGL26_COVERAGE)
  target_compile_options(lvgl26_warnings INTERFACE --coverage)
  target_link_options(lvgl26_warnings INTERFACE --coverage)
endif()
