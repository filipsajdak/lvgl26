# A precompiled header for the tests that has done lvgl26's first scans: the API, the style
# table and the members of a box, a label and a button. Off by default; it is measured in CI
# (docs/research/compile-diagnosis.md) and not offered to users.
option(LVGL26_WARM_PCH "Build the tests with a precompiled header that has done lvgl26's first scans" OFF)

# Gives every executable of the calling directory the warm header, except `skip`: a test whose
# flags differ (a precompiled header serves one set of flags), or one that measures what a file
# costs without it.
function(lvgl26_warm_pch)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "" "SKIP")
  add_library(lvgl26_warm_pch OBJECT ${PROJECT_SOURCE_DIR}/tests/warm_pch.cpp)
  target_link_libraries(lvgl26_warm_pch PRIVATE lvgl26::lvgl26 lvgl26_warnings)
  target_precompile_headers(lvgl26_warm_pch PRIVATE ${PROJECT_SOURCE_DIR}/tests/warm_pch.hpp)
  get_property(targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
  foreach(t IN LISTS targets)
    get_target_property(type ${t} TYPE)
    if(type STREQUAL "EXECUTABLE" AND NOT t IN_LIST arg_SKIP)
      target_precompile_headers(${t} REUSE_FROM lvgl26_warm_pch)
    endif()
  endforeach()
endfunction()
