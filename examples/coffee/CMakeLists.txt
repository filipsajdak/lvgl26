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
  # The picture of the step is the one the tutorial shows. LVGL v10 may draw other pixels.
  if(LVGL26_LVGL_REF STREQUAL "v9.6.0")
    set_tests_properties(example_${target} PROPERTIES FIXTURES_SETUP ${target}_picture)
    add_test(NAME example_${target}_picture
             COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/${target}_out/${target}.png
                     ${PROJECT_SOURCE_DIR}/docs/media/coffee-${number}.png)
    set_tests_properties(example_${target}_picture PROPERTIES FIXTURES_REQUIRED ${target}_picture)
  endif()
endforeach()
