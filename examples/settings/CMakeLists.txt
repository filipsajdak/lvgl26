cmake_minimum_required(VERSION 3.30)
project(settings LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(lvgl26 GIT_REPOSITORY https://github.com/filipsajdak/lvgl26 GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE lvgl26::lvgl26)
lvgl26_settings(app presets/microcontroller.json)  # one file for the whole program
