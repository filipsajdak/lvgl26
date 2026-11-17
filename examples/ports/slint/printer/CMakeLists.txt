# The printer panel, after the printer demo of Slint. Each page is its own source file, so
# the compiler works on one page at a time and a change touches one file.
add_library(printer_pages OBJECT
  copy.cpp scan.cpp print_page.cpp usb_page.cpp home_page.cpp settings_page.cpp ink_page.cpp
  queue.cpp shell.cpp)
target_link_libraries(printer_pages PUBLIC lvgl26::lvgl26 lvgl26_warnings)
target_compile_options(printer_pages PUBLIC --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(printer_pages PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Writes each page as a PNG file.
add_executable(printer_shot shot.cpp)
target_link_libraries(printer_shot PRIVATE printer_pages)

# The window needs SDL2.
if(LVGL26_BUILD_WINDOW_EXAMPLES)
  add_executable(printer window.cpp)
  target_link_libraries(printer PRIVATE printer_pages)
endif()
