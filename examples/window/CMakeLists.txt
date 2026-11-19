# Examples that open a window. They need SDL2, so they are off by default:
#   cmake --preset host-v96 -DLVGL26_BUILD_WINDOW_EXAMPLES=ON
# The window is our own adapter, examples/platform/sdl_window.hpp: it shows an lv::frame and
# feeds an lv::pointer. LVGL's own SDL driver is not used.
find_package(SDL2 REQUIRED)
# LVGL redraws and steps its animations every LV_DEF_REFR_PERIOD milliseconds. Its default
# of 33 gives 30 frames a second, which shows as stutter on a moving object.
target_compile_definitions(lvgl PUBLIC LV_USE_SNAPSHOT=1 LV_MEM_SIZE=8388608
                                       LV_DEF_REFR_PERIOD=8 LV_COLOR_FORMAT_DEFAULT=LV_COLOR_FORMAT_XRGB8888)
# LV_COLOR_FORMAT_DEFAULT: LVGL draws in 16-bit color by default, which shows as bands in a gradient.

# The window adapter, for every example that opens a window.
add_library(lvgl26_sdl_window INTERFACE)
target_include_directories(lvgl26_sdl_window INTERFACE ${PROJECT_SOURCE_DIR}/examples/platform)
target_link_libraries(lvgl26_sdl_window INTERFACE lvgl26::lvgl26 SDL2::SDL2)

# The applications of examples/wasm in a window, the same sources as in the browser;
# tests/window_app.cpp drives the counter with the same pointer.
foreach(application counter bounce)
  add_executable(window_${application} main.cpp)
  target_include_directories(window_${application} PRIVATE ${PROJECT_SOURCE_DIR}/examples/wasm/${application})
  target_compile_options(window_${application} PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/wasm/${application})
  target_link_libraries(window_${application} PRIVATE lvgl26_sdl_window lvgl26_warnings)
endforeach()
