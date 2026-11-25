# The coffee machine of the tutorial (docs/research/tutorial-coffee.md): one whole program for
# each step, each of which writes the picture of its step.
foreach(step 1-screen 2-values 3-pages 4-time 5-design)
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

# The machine's typeface is a copy of the site's Barlow, beside each design that embeds it.
foreach(copy 5-design/fonts 6-files/design/fonts)
  string(REGEX MATCH "^[0-9]+" number ${copy})
  foreach(file Barlow-Medium.ttf OFL.txt)
    add_test(NAME example_coffee_${number}_font_${file}
             COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/${copy}/${file}
                     ${PROJECT_SOURCE_DIR}/site/media/fonts/${file})
  endforeach()
endforeach()

# A step in files of its own: `main` writes the pictures, and the pages are one library, so the
# window does not compile them again. The other arguments are the files of the pages.
function(coffee_files number folder)
  set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${folder})
  set(target coffee_${number})
  list(TRANSFORM ARGN PREPEND ${dir}/)
  add_library(${target}_pages OBJECT ${ARGN})
  target_link_libraries(${target}_pages PUBLIC lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units)
  add_executable(${target} ${dir}/main.cpp)
  target_link_libraries(${target} PRIVATE ${target}_pages)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}_out)
  add_test(NAME example_${target} COMMAND ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}_out)
  if(LVGL26_BUILD_WINDOW_EXAMPLES)  # the window needs SDL2
    add_executable(${target}_window ${dir}/platform/window.cpp)
    target_link_libraries(${target}_window PRIVATE ${target}_pages lvgl26_sdl_window)
  endif()
endfunction()

# A picture that the program of step `number` writes, compared with one the tutorial shows.
function(coffee_picture number picture reference)
  if(LVGL26_LVGL_REF STREQUAL "v9.6.0")
    set_tests_properties(example_coffee_${number} PROPERTIES FIXTURES_SETUP coffee_${number}_pictures)
    add_test(NAME example_${picture}_picture
             COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/coffee_${number}_out/${picture}.png
                     ${PROJECT_SOURCE_DIR}/docs/media/${reference})
    set_tests_properties(example_${picture}_picture PROPERTIES FIXTURES_REQUIRED coffee_${number}_pictures)
  endif()
endfunction()

# Step 6 is the program of step 5 split into files by screen: its picture is step 5's.
coffee_files(6 6-files shell.cpp screens/drinks.cpp screens/cup.cpp screens/brewing.cpp screens/enjoy.cpp
             design/font_text_title.cpp)
coffee_picture(6 coffee_6 coffee-5.png)
