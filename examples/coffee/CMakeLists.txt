# The coffee machine of the tutorial (docs/research/tutorial-coffee.md): one whole program for
# each step, each of which writes the picture of its step.
foreach(step 1-screen 2-values 3-pages 4-time)
  string(REGEX MATCH "^[0-9]+" number ${step})
  set(target coffee_${number})
  add_executable(${target} ${step}/main.cpp)
  target_link_libraries(${target} PRIVATE lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units)
  target_compile_options(${target} PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/${step})
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}_out)
  add_test(NAME example_${target} COMMAND ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}_out)
endforeach()
