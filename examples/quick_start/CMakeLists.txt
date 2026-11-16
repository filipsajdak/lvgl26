cmake_minimum_required(VERSION 3.30)
project(quick_start LANGUAGES C CXX)

include(FetchContent)
set(LVGL26_UNITS ON)  # lvgl26 brings mp-units, for values with a unit
FetchContent_Declare(lvgl26 GIT_REPOSITORY https://github.com/filipsajdak/lvgl26 GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

add_executable(quick_start main.cpp)
target_link_libraries(quick_start PRIVATE lvgl26::lvgl26)
