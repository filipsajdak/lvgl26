# The program links lvgl26 through a library of its own, made before lvgl26 is added.
add_library(ui INTERFACE)
target_link_libraries(ui INTERFACE lvgl26::lvgl26)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE ui)
