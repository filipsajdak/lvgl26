# The coffee machine of the tutorial (docs/research/tutorial-coffee-draft.md): one whole
# program for each step, each of which writes the picture of its step.
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

# Steps 1 to 4 draw up to ten fonts each in the compiler. Two of them side by side, with the rest
# of a build, need more memory than a CI runner has: they compile one at a time.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS coffee_fonts=1)
set_target_properties(coffee_1 coffee_2 coffee_3 coffee_4 PROPERTIES JOB_POOL_COMPILE coffee_fonts)

# The machine's typeface is a copy of the site's Barlow, beside each design that embeds it:
# every file of such a folder must equal the site's file of the same name.
foreach(copy 1-screen/fonts 2-values/fonts 3-pages/fonts 4-time/fonts 5-design/fonts 6-files/design/fonts 7-service/design/fonts)
  string(REGEX MATCH "^[0-9]+" number ${copy})
  file(GLOB copied RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${copy} ${CMAKE_CURRENT_SOURCE_DIR}/${copy}/*)
  foreach(file IN LISTS copied)
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

# Step 4 shows the finished cup at 3 x target: its settings let the compiler decode that picture.
lvgl26_settings(coffee_4 4-time/lvgl26.json)
coffee_picture(4 coffee_4_enjoy coffee-4-enjoy.png)
# Under the sanitizers, decoding that picture takes 34 to 40 million operations, over GCC's default
# of 33,554,432: the step gets about twice its need. A source file's options come last.
set_source_files_properties(4-time/main.cpp PROPERTIES COMPILE_OPTIONS "-fconstexpr-ops-limit=67108864")

# Step 6 is the program of step 5 split into files by screen: its picture is step 5's.
coffee_files(6 6-files shell.cpp screens/drinks.cpp screens/cup.cpp screens/brewing.cpp screens/enjoy.cpp
             design/font_text_title.cpp design/font_text_body.cpp)
coffee_picture(6 coffee_6 coffee-5.png)

# Step 7 adds a page and the program's settings, which apply to the pages too.
coffee_files(7 7-service shell.cpp screens/drinks.cpp screens/cup.cpp screens/brewing.cpp screens/enjoy.cpp
             screens/service.cpp design/font_text_title.cpp design/font_text_body.cpp)
lvgl26_settings(coffee_7 7-service/lvgl26.json)
if(TARGET coffee_7_window)
  lvgl26_settings(coffee_7_window 7-service/lvgl26.json)
endif()
coffee_picture(7 coffee_7 coffee-7.png)
coffee_picture(7 coffee_7_service coffee-7-service.png)
