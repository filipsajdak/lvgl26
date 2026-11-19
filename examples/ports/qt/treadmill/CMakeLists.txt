# The treadmill panel, after the treadmill example of Qt Design Studio. The main screen is one
# tree in three looks, and each look is its own source file, so the compiler works on one look
# at a time and a change touches one file.
add_library(treadmill_pages OBJECT
  logic.cpp splash_page.cpp main_light.cpp main_dark.cpp main_sporty.cpp shell.cpp)
target_link_libraries(treadmill_pages PUBLIC lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units)
target_compile_options(treadmill_pages PUBLIC --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})
# The splash page decodes a PNG while compiling: 64 to 88 million operations, measured 2026-11-17.
# The default limit is 33.5 million, and the library no longer raises it for every user.
target_compile_options(treadmill_pages PRIVATE -fconstexpr-ops-limit=100000000)
target_include_directories(treadmill_pages PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Writes each scene as a PNG file.
add_executable(treadmill_shot shot.cpp)
target_link_libraries(treadmill_shot PRIVATE treadmill_pages)

# The window needs SDL2.
if(LVGL26_BUILD_WINDOW_EXAMPLES)
  add_executable(treadmill window.cpp)
  target_link_libraries(treadmill PRIVATE treadmill_pages lvgl26_sdl_window)
endif()
