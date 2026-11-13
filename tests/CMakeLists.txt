add_executable(smoke smoke.cpp)
target_link_libraries(smoke PRIVATE lvgl26::lvgl26)
add_test(NAME smoke COMMAND smoke)
