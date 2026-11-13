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
if(LVGL26_ANALYZE)
  target_compile_options(lvgl26_warnings INTERFACE -fanalyzer)
endif()

option(LVGL26_COVERAGE "Build the tests with coverage instrumentation" OFF)
if(LVGL26_COVERAGE)
  target_compile_options(lvgl26_warnings INTERFACE --coverage)
  target_link_options(lvgl26_warnings INTERFACE --coverage)
endif()
