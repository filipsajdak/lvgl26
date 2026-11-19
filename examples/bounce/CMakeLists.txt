# The bouncing ball of the site's "Motion, written as a coroutine". scene.hpp is the scene,
# with no platform; shot.cpp steps a clock and writes pictures; window.cpp opens a window.
add_executable(bounce_shot shot.cpp)
target_link_libraries(bounce_shot PRIVATE lvgl26::lvgl26 lvgl26_warnings)
target_compile_options(bounce_shot PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})

if(LVGL26_BUILD_WINDOW_EXAMPLES)
  add_executable(bounce window.cpp)
  target_link_libraries(bounce PRIVATE lvgl26_sdl_window lvgl26_warnings)
  target_compile_options(bounce PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})
endif()
