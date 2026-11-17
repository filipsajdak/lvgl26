# Examples that open a window. They need SDL2, so they are off by default:
#   cmake --preset host-v96 -DLVGL26_BUILD_WINDOW_EXAMPLES=ON
find_package(SDL2 REQUIRED)
# LVGL redraws and steps its animations every LV_DEF_REFR_PERIOD milliseconds. Its default
# of 33 gives 30 frames a second, which shows as stutter on a moving object.
target_compile_definitions(lvgl PUBLIC LV_USE_SDL=1 LV_USE_SNAPSHOT=1 LV_MEM_SIZE=8388608
                                       LV_DEF_REFR_PERIOD=8 LV_COLOR_FORMAT_DEFAULT=LV_COLOR_FORMAT_XRGB8888)
# LV_COLOR_FORMAT_DEFAULT: LVGL draws in 16-bit color by default, which shows as bands in a gradient.
target_link_libraries(lvgl PUBLIC SDL2::SDL2)
