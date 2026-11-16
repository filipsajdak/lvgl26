# A project that uses lvgl26 and turns editor support on, as docs/editor-support.md
# tells a reader to. The test editor_support configures a copy of it.
cmake_minimum_required(VERSION 3.30)
project(editor_consumer LANGUAGES C CXX)

include(FetchContent)
set(LVGL26_UNITS ON)
FetchContent_Declare(lvgl26 GIT_REPOSITORY https://github.com/filipsajdak/lvgl26 GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE lvgl26::lvgl26)

# A second program, with a design token file next to it.
add_executable(themed theme.cpp)
target_link_libraries(themed PRIVATE lvgl26::lvgl26)

lvgl26_editor_support()
