# Where #embed finds the test data.
add_library(lvgl26_test_data INTERFACE)
target_compile_options(lvgl26_test_data INTERFACE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/data)
# The Barlow files of the site are the test fonts: `#embed "fonts/Barlow-Medium.ttf"`.
target_compile_options(lvgl26_test_data INTERFACE --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
# The compile-fail tests that use quantities need mp-units too.
target_link_libraries(lvgl26_test_data INTERFACE mp-units::mp-units)

include(${PROJECT_SOURCE_DIR}/cmake/compile_fail.cmake)

function(lvgl26_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE lvgl26::lvgl26 lvgl26_warnings)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# A file that must not compile, and the message the user must see.
# The test runs the compiler itself, with the flags of the target fail_<name>, and only
# as far as the errors (-fsyntax-only). It does not go through `cmake --build`: two of
# those cannot run in one build directory at once, and these tests are most of the
# suite, so ctest must be able to run them side by side.
function(lvgl26_compile_fail name message)
  set(target fail_${name})
  add_library(${target} OBJECT EXCLUDE_FROM_ALL fail/${name}.cpp)
  target_link_libraries(${target} PRIVATE lvgl26::lvgl26 lvgl26_test_data)
  add_test(NAME ${target}
           COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX26_EXTENSION_COMPILE_OPTION} -fsyntax-only
                   "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,PREPEND,-I>"
                   "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>,PREPEND,-D>"
                   "$<TARGET_PROPERTY:${target},COMPILE_OPTIONS>"
                   ${CMAKE_CURRENT_SOURCE_DIR}/fail/${name}.cpp
           COMMAND_EXPAND_LISTS)
  set_tests_properties(${target} PROPERTIES PASS_REGULAR_EXPRESSION "${message}" LABELS compile-fail)
endfunction()

# The same, for a message that the library writes: the text is the whole message, as the user reads it, and the
# test fails when the message differs or has text after it (cmake/compile_fail.cmake says how a text becomes a
# regular expression). `form` is where GCC prints it: constexpr (a consteval error), static_assert or deleted.
# The text is kept in the test's properties for tools/compile_fail_messages.py.
function(lvgl26_compile_fail_message name form text)
  lvgl26_message_regex(regex "${form}" "${text}")
  lvgl26_compile_fail(${name} "${regex}")
  set_tests_properties(fail_${name} PROPERTIES LVGL26_MESSAGE "${text}" LVGL26_MESSAGE_FORM "${form}")
endfunction()

lvgl26_test(smoke)
lvgl26_test(icons)
lvgl26_test(icons_font)
lvgl26_test(api_helpers)
lvgl26_test(nearest)
lvgl26_test(all_header)
# The settings reader, and a program whose settings come from a file.
lvgl26_test(settings)
lvgl26_test(json_fault)
add_library(settings_library STATIC settings/library.cpp)
target_link_libraries(settings_library PUBLIC lvgl26::lvgl26 PRIVATE lvgl26_warnings)
add_executable(settings_file settings_file.cpp)
target_link_libraries(settings_file PRIVATE settings_library lvgl26_warnings)
lvgl26_settings(settings_file settings/project.json)
add_test(NAME settings_file COMMAND settings_file)
# The check before the link: an object with the mark of other settings stops it.
add_library(settings_marks OBJECT settings/mark_a.cpp settings/mark_b.cpp)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/settings_marks_all.txt
     CONTENT "$<JOIN:$<TARGET_OBJECTS:settings_marks>,\n>\n")
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/settings_marks_own.txt
     CONTENT "$<LIST:FILTER,$<TARGET_OBJECTS:settings_marks>,INCLUDE,mark_a>\n")
foreach(case all own)
  add_test(NAME settings_scan_${case}
           COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DHASH=aaaaaaaaaaaaaaaa -DNAME=lvgl26.json
                   -DLIST=${CMAKE_CURRENT_BINARY_DIR}/settings_marks_${case}.txt -P ${PROJECT_SOURCE_DIR}/cmake/settings_scan.cmake)
endforeach()
set_tests_properties(settings_scan_all PROPERTIES PASS_REGULAR_EXPRESSION
  "mark_b.cpp.o was compiled with other settings \\(lvgl26_settings_bbbbbbbbbbbbbbbb\\) than this program, whose settings file is lvgl26.json")
# The presets of the setup skill (skills/settings): each builds and runs the example.
set(LVGL26_SETTINGS_EXAMPLE ${CMAKE_CURRENT_SOURCE_DIR}/../examples/settings)
foreach(preset desktop microcontroller browser)
  add_executable(settings_${preset} ${LVGL26_SETTINGS_EXAMPLE}/main.cpp)
  target_link_libraries(settings_${preset} PRIVATE lvgl26::lvgl26 lvgl26_warnings)
  lvgl26_settings(settings_${preset} ${LVGL26_SETTINGS_EXAMPLE}/presets/${preset}.json)
  add_test(NAME examples_settings_${preset} COMMAND settings_${preset})
endforeach()
# The restrictions change no code: one file with none and with all four.
add_library(settings_cost_open OBJECT settings_zero_cost.cpp)
add_library(settings_cost_closed OBJECT settings_zero_cost.cpp)
target_compile_options(settings_cost_closed PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/settings/all_forbidden.hpp)
foreach(target settings_cost_open settings_cost_closed)
  target_link_libraries(${target} PRIVATE lvgl26::lvgl26 lvgl26_warnings)
  target_compile_options(${target} PRIVATE -O2 -g0)
endforeach()
add_test(NAME settings_zero_cost
         COMMAND ${CMAKE_COMMAND} -DOBJDUMP=${CMAKE_OBJDUMP} -DNM=${CMAKE_NM}
                 "-DOPEN=$<TARGET_OBJECTS:settings_cost_open>" "-DCLOSED=$<TARGET_OBJECTS:settings_cost_closed>"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/settings_same_objects.cmake)
# The library in a program without exceptions and run-time type information.
lvgl26_test(no_exceptions)
target_compile_options(no_exceptions PRIVATE -fno-exceptions -fno-rtti)
lvgl26_test(l1_views)
lvgl26_test(json)
lvgl26_test(runtime_ui)
lvgl26_test(match)
lvgl26_test(for_each)
lvgl26_test(mounted_storage)
lvgl26_test(pointer)
lvgl26_test(gesture)
lvgl26_test(input_queue)
# back() of an empty queue ends the program.
add_test(NAME input_queue_violation_back_of_empty
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:input_queue> -DCASE=back_of_empty
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
# The application's loop: sources, a window that feeds a pointer and closes, a hidden display.
lvgl26_test(app_loop)
add_test(NAME app_loop_violation_display_of_c_display
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:app_loop> -DCASE=display_of_c_display
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
# The application of examples/window, fed by an lv::pointer as its window feeds it.
lvgl26_test(window_app)
# The host build of examples/wasm: its platform and host program with each application,
# replaying the application's script. The nightly job builds the same sources for
# WebAssembly and compares the frames; this run catches a library change that breaks them.
foreach(application counter bounce)
  set(script_inc ${CMAKE_CURRENT_BINARY_DIR}/wasm_${application}/script.inc)
  add_custom_command(OUTPUT ${script_inc}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/wasm_${application}
    COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/wasm/make-script.py
            ${PROJECT_SOURCE_DIR}/examples/wasm/${application}/script.json > ${script_inc}
    DEPENDS ${PROJECT_SOURCE_DIR}/examples/wasm/${application}/script.json ${PROJECT_SOURCE_DIR}/tools/wasm/make-script.py)
  add_executable(wasm_host_${application} ${PROJECT_SOURCE_DIR}/examples/wasm/platform.cpp
                 ${PROJECT_SOURCE_DIR}/examples/wasm/host_main.cpp ${script_inc})
  target_include_directories(wasm_host_${application} PRIVATE ${PROJECT_SOURCE_DIR}/examples/wasm/${application}
                             ${PROJECT_SOURCE_DIR}/examples/wasm ${CMAKE_CURRENT_BINARY_DIR}/wasm_${application})
  target_compile_options(wasm_host_${application} PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/wasm/${application})
  target_link_libraries(wasm_host_${application} PRIVATE lvgl26::lvgl26 lvgl26_warnings)
  add_test(NAME wasm_host_${application}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:wasm_host_${application}>
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/run_quiet.cmake)
endforeach()
# A polled pointer without a function ends the program.
add_test(NAME pointer_violation_no_function
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:pointer> -DCASE=no_function
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
target_link_libraries(mounted_storage PRIVATE lvgl26_test_data)  # look_tokens.json
lvgl26_test(one_of)
lvgl26_test(one_of_region)
lvgl26_test(deferred)
lvgl26_test(shown)
lvgl26_test(shown_transitions)
# The way back plays .enter_back and .leave_back; the pictures are half way through a slide each way.
add_executable(shown_direction shown_direction.cpp)
target_link_libraries(shown_direction PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME shown_direction COMMAND shown_direction A:${CMAKE_CURRENT_SOURCE_DIR}/ref/shown_direction_forward.png
                                                      A:${CMAKE_CURRENT_SOURCE_DIR}/ref/shown_direction_back.png
                                                      A:${CMAKE_CURRENT_SOURCE_DIR}/ref/shown_direction_top.png)
lvgl26_test(shown_kept)
# A wrong call on a slot ends the program: unmount() of an empty slot, the handle of a tree
# that is not mounted (also after an arm switched the slot), error() of a slot that holds one.
foreach(case unmount_empty get_other held_after_remount error_of_mounted)
  add_test(NAME one_of_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:one_of> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
# unmount() of a tree that is not mounted, mount() of one that is, a view of a tree that is not mounted, and
# mount_in() in memory that is too small or not aligned, end the program.
foreach(case unmount_not_mounted mount_twice unmount_after_parent get_not_mounted mount_in_too_small mount_in_misaligned)
  add_test(NAME mounted_storage_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:mounted_storage> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
lvgl26_test(look)
lvgl26_test(start_style)
lvgl26_test(style_add)
target_link_libraries(look PRIVATE lvgl26_test_data)
# A look whose styles follow the mode of a display; the pictures are both modes.
add_executable(mode_look mode_look.cpp)
target_link_libraries(mode_look PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME mode_look COMMAND mode_look A:${CMAKE_CURRENT_SOURCE_DIR}/ref/mode_look_day.png
                                          A:${CMAKE_CURRENT_SOURCE_DIR}/ref/mode_look_dark.png)
# Two files of one program name looks, with the registration in a header they share.
add_executable(look_link look_link_main.cpp look_link_cards.cpp)
target_link_libraries(look_link PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME look_link COMMAND look_link)
# Fonts drawn in files of their own: two files draw one font each, a third names them.
add_executable(fonts_split fonts_split_main.cpp fonts_split_label.cpp fonts_split_small.cpp)
target_link_libraries(fonts_split PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME fonts_split COMMAND fonts_split)
# The build checks that every font a program names has a file that draws it (tools/check_fonts.py).
add_test(NAME check_fonts_tool COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_fonts.py)
add_test(NAME check_fonts_program
         COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_fonts.py ${CMAKE_CURRENT_SOURCE_DIR}/fonts_split_main.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/fonts_split_label.cpp ${CMAKE_CURRENT_SOURCE_DIR}/fonts_split_small.cpp)
add_test(NAME check_fonts_missing
         COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_fonts.py ${CMAKE_CURRENT_SOURCE_DIR}/fonts_split_main.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/fonts_split_label.cpp)
set_tests_properties(check_fonts_missing PROPERTIES PASS_REGULAR_EXPRESSION
                     "fonts_split_main.cpp:[0-9]+: the font 'text.small' has no file that draws it. Add font_text_small.cpp")
lvgl26_test(state)
lvgl26_test(units)
lvgl26_test(values)
# An angle that breaks its contract at run time ends the program.
foreach(case fraction number whole)
  add_test(NAME units_decidegrees_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:units> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
target_link_libraries(state PRIVATE mp-units::mp-units)
lvgl26_test(computed)
lvgl26_test(concepts)
target_link_libraries(concepts PRIVATE mp-units::mp-units)
lvgl26_test(handlers)
lvgl26_test(lists)
lvgl26_test(lists_selection)
target_link_libraries(lists_selection PRIVATE lvgl26_test_data)
# An index out of range, or a list changed while its views follow a change, ends the program.
foreach(case index erase move move_from_end move_from_far move_to_end insert set telling)
  add_test(NAME lists_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lists> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
target_link_libraries(computed PRIVATE mp-units::mp-units)
lvgl26_test(state_plain)
lvgl26_test(watchers)
lvgl26_test(angle_units)
lvgl26_test(task)
lvgl26_test(static_text)
lvgl26_test(setter_order)
lvgl26_test(options)
# LVGL says which setters take options separated by line breaks in its doc comments; the names
# that lvgl26 goes by must agree with them.
lvgl26_test(options_docs)
lvgl26_test(api_table)
# A state of an enum given to set() at run time breaks its contract.
add_test(NAME options_violation_enum_state_in_set
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:options> -DCASE=set
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
lvgl26_test(animate)
# Local style values: of the main part with the plain call, of a part and a state with lv::in.
lvgl26_test(animate_style)
lvgl26_test(enum_names)
lvgl26_test(unsigned_values)
add_test(NAME unsigned_values_violation_negative
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:unsigned_values> -DCASE=negative
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
lvgl26_test(application)
lvgl26_test(theme)
# One theme's styles take the place of another's in the widgets that exist.
lvgl26_test(theme_restyle)
# A display that follows a state: Signal's day and dark, with the pictures of both, and two themes of different shapes.
add_executable(theme_follow theme_follow.cpp)
target_link_libraries(theme_follow PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME theme_follow COMMAND theme_follow A:${CMAKE_CURRENT_SOURCE_DIR}/ref/theme_follow_day.png
                                                A:${CMAKE_CURRENT_SOURCE_DIR}/ref/theme_follow_dark.png)
add_test(NAME theme_follow_violation_shape
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:theme_follow> -DCASE=shape
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
# Only a build without LVGL's default theme can run it (preset host-v96-no-theme).
if(LVGL26_NO_DEFAULT_THEME)
  lvgl26_test(no_default_theme)
endif()
lvgl26_test(scaled_font)
target_link_libraries(scaled_font PRIVATE lvgl26_test_data)
# A font size is more than nothing.
foreach(case zero negative)
  add_test(NAME scaled_font_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:scaled_font> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
lvgl26_test(signal)
# Signal's typefaces. LVGL v9.6 and the v10 pin render the same pixels, so one reference
# serves both.
add_executable(signal_type signal_type.cpp)
target_link_libraries(signal_type PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME signal_type COMMAND signal_type A:${CMAKE_CURRENT_SOURCE_DIR}/ref/signal_type_dark.png
                                              A:${CMAKE_CURRENT_SOURCE_DIR}/ref/signal_type_day.png)
# The same test with the fonts linked from lvgl26::signal_type (lv/signal_fonts.hpp): the same pictures.
add_executable(signal_linked signal_linked.cpp)
target_link_libraries(signal_linked PRIVATE lvgl26::signal_type lvgl26_warnings)
add_test(NAME signal_linked COMMAND signal_linked A:${CMAKE_CURRENT_SOURCE_DIR}/ref/signal_type_dark.png
                                                  A:${CMAKE_CURRENT_SOURCE_DIR}/ref/signal_type_day.png)
# Signal's tell-tale lamp colours, a header of its own.
lvgl26_test(signal_lamps)
# Signal's durations and easings, a header of its own.
lvgl26_test(signal_motion)
lvgl26_test(signal_gesture)
# A header embeds only what lies next to it, so lv/signal_type.hpp has its own copies of
# the font files of the site. They must stay the same files.
foreach(file Barlow-Medium.ttf BarlowCondensed-SemiBold.ttf BarlowCondensed-Bold.ttf OFL.txt)
  add_test(NAME signal_type_file_${file}
           COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_SOURCE_DIR}/include/lv/signal/fonts/${file}
                   ${PROJECT_SOURCE_DIR}/site/media/fonts/${file})
endforeach()
target_link_libraries(theme PRIVATE lvgl26_test_data)
lvgl26_test(style_selectors)
# A layer of lv::on in a style for another part, and two layered styles merged at run time.
foreach(case part_slot merge)
  add_test(NAME style_selectors_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:style_selectors> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
# LVGL's state is global: a second application ends the program.
add_test(NAME application_violation_twice
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:application> -DCASE=twice
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
# lv::ui_scheduler is the one part that other threads call.
find_package(Threads REQUIRED)
lvgl26_test(ui_scheduler)
target_link_libraries(ui_scheduler PRIVATE Threads::Threads)

# The agent kit writes its pictures into a directory of its own. It counts the files
# there, so no other test may write into it.
add_executable(agent_kit agent_kit.cpp)
target_link_libraries(agent_kit PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/agent_kit_files)
add_test(NAME agent_kit COMMAND agent_kit ${CMAKE_CURRENT_BINARY_DIR}/agent_kit_files)

# The example host answers a recorded session: it loads a screen, patches it,
# dumps the tree with the new value and writes a screenshot.
set(LVGL26_AGENT_EXAMPLE ${CMAKE_CURRENT_SOURCE_DIR}/../examples/agent)
add_executable(mcp_host ${LVGL26_AGENT_EXAMPLE}/mcp_host.cpp)
target_link_libraries(mcp_host PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME examples_mcp_host
         COMMAND mcp_host ${LVGL26_AGENT_EXAMPLE}/session.jsonl ${CMAKE_CURRENT_BINARY_DIR})
# The server only makes new files, so the screenshot of the last run goes first.
add_test(NAME examples_mcp_host_clean
         COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/mcp_host_screen.png)
set_tests_properties(examples_mcp_host_clean PROPERTIES FIXTURES_SETUP mcp_host_files)
set_tests_properties(examples_mcp_host PROPERTIES FIXTURES_REQUIRED mcp_host_files
  PASS_REGULAR_EXPRESSION
  "\"id\":1,\"result\":{\"protocolVersion\":\"2025-11-25\".*\"text\":\"loaded\".*\"text\":\"applied\".*value[^0-9]+60.*wrote [^\"]*/mcp_host_screen.png")

# LVGL v9.6 and the v10 pin render the same pixels, so one reference serves both.
add_executable(l2_tree l2_tree.cpp)
target_link_libraries(l2_tree PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME l2_tree COMMAND l2_tree A:${CMAKE_CURRENT_SOURCE_DIR}/ref/slice.png)
# The call form builds the node that the subscript form builds.
lvgl26_test(call_form)
# The members of a props struct are in the order a tree is written in.
lvgl26_test(props_order)

# A tree of 112 widgets, and the guard of its compile time. The limit is the number of
# steps the compiler may take in one constant evaluation; GCC's default is 33554432.
# This file needs 5.8 million, on both LVGL versions; the limit below is 6.5 million. With nodes that held a whole
# props struct, as they first did, it needed more than 170 million, and a tree of 100
# widgets took the compiler 8 GB of memory. The limit leaves room for LVGL to grow,
# not for that to come back.
# A file that includes <lv/lv.hpp> and names nothing, under a fixed operation limit: a header
# that does a scan where it is read, in every file, stops it compiling. The largest evaluation
# of such a file needed 12,000 to 14,000 operations on 2026-11-20, once lv/lv.hpp became the core
# (25,000 to 26,000 before); the limit is about twice that, as large_tree's. Raise it only with a measurement:
# python3 tools/compile_report.py --ops build/host-v96 tests/include_only.cpp.
set(lvgl26_include_only_ops 26000)
lvgl26_test(include_only)
set_source_files_properties(include_only.cpp PROPERTIES
  COMPILE_OPTIONS "-fno-analyzer;-fconstexpr-ops-limit=${lvgl26_include_only_ops}")

add_executable(large_tree large_tree.cpp)
target_link_libraries(large_tree PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
# Not for the analyzer: it follows every path through the 112 functions that build this
# tree, which takes it minutes. They are the functions the small trees are built with,
# and those it does check. A source file's options come last, after -fanalyzer, and
# after the limit that lvgl26 raises for its users: GCC keeps the last one it reads.
set_source_files_properties(large_tree.cpp PROPERTIES
  COMPILE_OPTIONS "-fno-analyzer;-fconstexpr-ops-limit=6500000")
add_test(NAME large_tree COMMAND large_tree)

add_executable(tokens tokens.cpp)
target_link_libraries(tokens PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME tokens COMMAND tokens A:${CMAKE_CURRENT_SOURCE_DIR}/ref/tokens_dark.png
                                    A:${CMAKE_CURRENT_SOURCE_DIR}/ref/tokens_light.png)

# The example ports, each compared pixel for pixel with LVGL's C original.
set(LVGL26_C_EXAMPLES ${lvgl_SOURCE_DIR}/examples/get_started)
add_library(lvgl_c_get_started STATIC EXCLUDE_FROM_ALL
  ${LVGL26_C_EXAMPLES}/get_started_hello_world/lv_example_get_started_hello_world.c
  ${LVGL26_C_EXAMPLES}/get_started_slider/lv_example_get_started_slider.c
  ${LVGL26_C_EXAMPLES}/get_started_styles/lv_example_get_started_styles.c
  ${LVGL26_C_EXAMPLES}/lv_example_get_started_button.c)
target_link_libraries(lvgl_c_get_started PRIVATE lvgl::lvgl)
target_compile_definitions(lvgl_c_get_started PRIVATE LV_BUILD_EXAMPLES=1)

set(LVGL26_EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/lvgl/get_started)
add_executable(examples_get_started examples_get_started.cpp
  ${LVGL26_EXAMPLES}/hello_world.cpp ${LVGL26_EXAMPLES}/slider.cpp
  ${LVGL26_EXAMPLES}/styles.cpp ${LVGL26_EXAMPLES}/button.cpp)
target_compile_options(examples_get_started PRIVATE --embed-dir=${LVGL26_EXAMPLES})
target_link_libraries(examples_get_started PRIVATE
  lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units lvgl_c_get_started)
add_test(NAME examples_get_started COMMAND examples_get_started)

# LVGL's examples/widgets, one test per widget folder. The arguments after the
# folder are the ported examples: lv_example_<folder>_<name>.c is compared with
# examples/lvgl/widgets/<folder>_<name>.cpp. ASSETS names the pictures they use.
set(LVGL26_C_WIDGETS ${lvgl_SOURCE_DIR}/examples/widgets)
set(LVGL26_WIDGETS ${CMAKE_CURRENT_SOURCE_DIR}/../examples/lvgl/widgets)
function(lvgl26_widget_ports folder)
  cmake_parse_arguments(arg "" "" "ASSETS" ${ARGN})
  set(originals)
  set(ports)
  foreach(name ${arg_UNPARSED_ARGUMENTS})
    list(APPEND originals ${LVGL26_C_WIDGETS}/${folder}/lv_example_${folder}_${name}.c)
    list(APPEND ports ${LVGL26_WIDGETS}/${folder}_${name}.cpp)
  endforeach()
  # The pictures an example draws with come from LVGL's examples/assets.
  foreach(asset ${arg_ASSETS})
    list(APPEND originals ${lvgl_SOURCE_DIR}/examples/assets/${asset}.c)
  endforeach()
  add_library(lvgl_c_widgets_${folder} STATIC EXCLUDE_FROM_ALL ${originals})
  target_link_libraries(lvgl_c_widgets_${folder} PRIVATE lvgl::lvgl)
  target_compile_definitions(lvgl_c_widgets_${folder} PRIVATE LV_BUILD_EXAMPLES=1)

  add_executable(examples_widgets_${folder} examples_widgets_${folder}.cpp ${ports})
  target_compile_options(examples_widgets_${folder} PRIVATE --embed-dir=${LVGL26_WIDGETS})
  target_link_libraries(examples_widgets_${folder} PRIVATE
    lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units lvgl_c_widgets_${folder})
  add_test(NAME examples_widgets_${folder} COMMAND examples_widgets_${folder})
endfunction()

lvgl26_widget_ports(label bind_text long_mode recolor set_text_newline styling text_align)
lvgl26_widget_ports(button basic event states styling)
lvgl26_widget_ports(arc bind_value change_rate event img_indicator mode pie_chart rotation
                    set_angles styling value_range
                    ASSETS img_arc_bg img_arc_indicator)
lvgl26_widget_ports(bar bind_value img_indicator modes orientation styling value_range
                    ASSETS img_bar_bg img_bar_indicator)
lvgl26_widget_ports(slider bind_state bind_value event img_indicator modes reversed_direction
                    styling value_range_orientation
                    ASSETS img_bar_bg img_bar_indicator)
lvgl26_widget_ports(switch event orientation state styling)
lvgl26_widget_ports(checkbox bind_checked event states styling text)
lvgl26_widget_ports(led brightness color styling)
lvgl26_widget_ports(line points)
lvgl26_widget_ports(obj bind_style)
lvgl26_widget_ports(buttonmatrix event map one_checked popover recolor styling)
lvgl26_widget_ports(dropdown bind_value direction event text)
# LVGL draws a roller with a left shift of a negative number (lv_roller.c, draw_main), which a
# sanitizer stops on. The picture is compared in the builds that have none.
if(NOT LVGL26_SANITIZE)
  lvgl26_widget_ports(roller bind_value event options styling visible_rows
                      ASSETS font_example_large)
endif()
lvgl26_widget_ports(spinbox bind_value format rollover styling value)
lvgl26_widget_ports(spinner basic)
lvgl26_widget_ports(textarea cursor placeholder)
lvgl26_widget_ports(image inner_align src transformations ASSETS img_example_lvgl_logo)
lvgl26_widget_ports(scale modes range styling ticks)
lvgl26_widget_ports(keyboard modes popovers textarea)

add_executable(font font.cpp)
target_compile_options(font PRIVATE --embed-dir=${lvgl_SOURCE_DIR})  # two of LVGL's own font files
target_link_libraries(font PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font COMMAND font A:${CMAKE_CURRENT_SOURCE_DIR}/ref/font.png)
# Characters beyond the default set, and a picture of the euro sign and the middle dot.
add_executable(font_extra font_extra.cpp)
target_link_libraries(font_extra PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font_extra COMMAND font_extra A:${CMAKE_CURRENT_SOURCE_DIR}/ref/font_extra.png)
# The fonts are the bytes that one evaluation of the whole font made.
add_executable(font_bytes font_bytes.cpp)
target_compile_options(font_bytes PRIVATE --embed-dir=${lvgl_SOURCE_DIR})
target_link_libraries(font_bytes PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font_bytes COMMAND font_bytes)

add_executable(image image.cpp)
target_link_libraries(image PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME image COMMAND image A:${CMAKE_CURRENT_SOURCE_DIR}/ref/image.png)
lvgl26_test(image_alone)
# A picture made by the compiler is named by its file, not by its pixels: a name of a million characters
# stops Apple's linker (tools/check_symbols.py). The object file of a large picture in the 32-bit form
# is read by the test image_symbols.
add_library(image_symbols OBJECT image_symbols.cpp)
target_link_libraries(image_symbols PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_executable(encoded_image encoded_image.cpp)
target_link_libraries(encoded_image PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME encoded_image COMMAND encoded_image A:${CMAKE_CURRENT_SOURCE_DIR}/ref/encoded_image.png)
target_link_libraries(image_alone PRIVATE lvgl26_test_data)

add_executable(styles styles.cpp)
target_link_libraries(styles PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME styles COMMAND styles A:${CMAKE_CURRENT_SOURCE_DIR}/ref/styled.png)

# The printer panel of examples/ports/slint/printer: each page, compared with a reference picture.
add_executable(ports_printer ports_printer.cpp)
target_link_libraries(ports_printer PRIVATE printer_pages lvgl26_warnings)
add_test(NAME ports_printer COMMAND ports_printer A:${CMAKE_CURRENT_SOURCE_DIR}/ref/printer_)
# What the port costs in heap and in widgets made and deleted while the queue changes.
add_executable(printer_cost printer_cost.cpp)
target_link_libraries(printer_cost PRIVATE printer_pages lvgl26_warnings)
add_test(NAME printer_cost COMMAND printer_cost)
# The print queue of that port, built with lv::each, against the way the port builds it.
add_executable(lists_printer lists_printer.cpp)
target_link_libraries(lists_printer PRIVATE printer_pages lvgl26_warnings)
add_test(NAME lists_printer COMMAND lists_printer)

# The pictures of docs/port-slint-printer.md are the ones the port's screenshot tool writes.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ports_printer_out)
add_test(NAME ports_printer_shot COMMAND printer_shot ${CMAKE_CURRENT_BINARY_DIR}/ports_printer_out)
set_tests_properties(ports_printer_shot PROPERTIES FIXTURES_SETUP ports_printer_pictures)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  foreach(scene home copy)
    add_test(NAME ports_printer_picture_${scene}
             COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/ports_printer_out/${scene}.png
                     ${PROJECT_SOURCE_DIR}/docs/media/printer-${scene}.png)
    set_tests_properties(ports_printer_picture_${scene} PROPERTIES FIXTURES_REQUIRED ports_printer_pictures)
  endforeach()
endif()

# The treadmill panel of examples/ports/qt/treadmill: each scene, compared with a reference picture.
add_executable(ports_treadmill ports_treadmill.cpp)
target_link_libraries(ports_treadmill PRIVATE treadmill_pages lvgl26_warnings)
add_test(NAME ports_treadmill COMMAND ports_treadmill ${CMAKE_CURRENT_SOURCE_DIR}/ref/treadmill_)

lvgl26_compile_fail_message(dump_not_a_sink deleted "dump takes a sink: something to call with a std::string_view, for example [&](std::string_view piece) { text += piece; }")
lvgl26_compile_fail(dump_loaded_not_a_sink "dump takes a sink: something to call with a std::string_view")
lvgl26_compile_fail_message(unknown_property constexpr "lv: widget 'label' has no property 'txt' to set. Its own: <<names>>. Every 'lv_obj_set_*' property of obj works too.")
lvgl26_compile_fail_message(unknown_widget constexpr "lv: there is no widget 'lable'. Valid: <<widgets>>.")
lvgl26_compile_fail(big_capture "an event handler may capture nothing, or one pointer-sized")
# A part that constrains its parameters with the public concepts (docs/concepts.md).
# The whole message of a handler that holds state, sentence by sentence: each says what to do.
string(CONCAT handler_state_message "It takes a function or a lambda that captures nothing: `&function`, `\\[\\] \\{ \\.\\.\\. \\}` or "
    "`\\[\\]\\(lv::event e\\) \\{ \\.\\.\\. \\}`, or for on_swiped `\\[\\]\\(lv::swipe d\\) \\{ \\.\\.\\. \\}`\\. A handler in a constexpr tree is a plain function pointer, "
    "so it cannot hold state: name the state as a template argument "
    "\\(`template <lv::dependency auto& Count>`\\) and use it in the lambda, or add the handler to "
    "the view with on<LV_EVENT_\\.\\.\\.>\\(\\)\\.")
lvgl26_compile_fail(handler_capturing_lambda "${handler_state_message}")
lvgl26_compile_fail(part_handler_capturing_lambda "without_captures__name_the_state_as_a_template_argument")
lvgl26_compile_fail(part_event_handler_capturing_lambda "${handler_state_message}")
lvgl26_compile_fail(part_tree_handler_not_callable "requires +tree_handler<")
lvgl26_compile_fail(part_handler_not_callable "requires +event_handler<")
lvgl26_compile_fail(part_dependency_plain_value "requires +dependency<")
lvgl26_compile_fail(part_dependency_template_argument "lv::dependency<")
lvgl26_compile_fail(part_dependency_requires_clause "dependency<")
lvgl26_compile_fail(part_font_not_a_font "requires +font_value<")
lvgl26_compile_fail(part_token_wrong_value "token_of<")
lvgl26_compile_fail(part_tree_node_not_a_node "requires +tree_node<")
lvgl26_compile_fail(part_style_not_a_style "requires +style_value<")
lvgl26_compile_fail(unknown_id "the tree has no id 'titel'. Its ids: title")
lvgl26_compile_fail(duplicate_id "the id 'a' is used twice in one tree")
lvgl26_compile_fail_message(list_without_rows static_assert "lv: a list holds at least one row: lv::list<track, 8>.")
lvgl26_compile_fail_message(each_two_rows static_assert "lv: lv::each takes one row template. Put several widgets in one: lv::each(...)[lv::row[a, b]].")
lvgl26_compile_fail_message(each_row_storage static_assert "lv: a row of lv::each cannot hold a node that keeps run-time storage yet: lv::each builds its rows outside the tree's storage. Put that node outside the row template.")
lvgl26_compile_fail(mounted_detach_storage "keeps run-time storage inside its lv::mounted, so its widgets cannot")
lvgl26_compile_fail(place_storage "lv::place leaves the widgets to their parent, and this tree keeps run-time")
lvgl26_compile_fail(one_of_unknown_tree "other is not a tree of this lv::one_of. Its trees: home, settings.")
lvgl26_compile_fail_message(visit_no_arm_for_tree constexpr "lv: no arm of visit takes the tree settings. Add an arm that takes lv::held<settings>, or one that takes auto.")
lvgl26_compile_fail(visit_arm_never_called "arm 2 of visit can never be called.*The slot's trees: home, settings.")
lvgl26_compile_fail_message(visit_arm_takes_mounted constexpr "lv: an arm of visit takes lv::held<home>, not lv::mounted<home>: write [](lv::held<home> page). The handle checks that the slot still holds home each time it is used.")
lvgl26_compile_fail_message(visit_arm_takes_tree_by_value constexpr "lv: an arm of visit takes lv::held<home>, not lv::mounted<home>: write [](lv::held<home> page). The handle checks that the slot still holds home each time it is used.")
lvgl26_compile_fail_message(visit_arm_auto_reference constexpr "lv: arm 2 of visit cannot take lv::held<settings>. Take the handle by value: [](auto page), or [](lv::held<settings> page).")
lvgl26_compile_fail(visit_named_no_conversion "arm 2 of visit returns std::basic_string_view<char>, which does not convert to int, the result that visit<R> names")
lvgl26_compile_fail(visit_tree_twice "arm 2 of visit can never be called.*each tree once")
lvgl26_compile_fail_message(visit_no_common_type constexpr "lv: the arms of visit return types with no common type: int and const char*. Make the arms return types that convert to one type, as std::visit needs.")
lvgl26_compile_fail(one_of_tree_twice "the tree home is listed twice in this lv::one_of")
lvgl26_compile_fail(loop_not_a_source "a source of the application's loop has a member function poll\\(\\)")
lvgl26_compile_fail(loop_temporary_application "a source joins an application that outlives it: declare the application first, then the source")
lvgl26_compile_fail(pointer_polled_press "this pointer is polled: LVGL asks its function. Use lv::pointer mouse\\{display\\} to feed one")
lvgl26_compile_fail(pointer_temporary_display "the pointer keeps a pointer to its display: declare the display first, then the pointer")
lvgl26_compile_fail(pointer_capturing_lambda "a polled pointer takes a function lv::pointer_sample\\(\\): LVGL calls it at each read")
lvgl26_compile_fail(pointer_queue_too_large "Capacity <= .*most_pointer_samples")
lvgl26_compile_fail(storage_kind_shape "a node kind with a member `storage` declares it as")
lvgl26_compile_fail_message(each_without_row static_assert "lv: lv::each needs a row template: lv::each(tracks, &track::id)[lv::label(...)].")
lvgl26_compile_fail_message(each_plain_key static_assert "lv: a key names one row, and is compared as a state compares its value. Give it a named type, `struct track_id { int value; };`, or an enum. To allow plain integers, set rules.raw_integer_state in the settings.")
lvgl26_compile_fail_message(each_not_a_list deleted "lv: lv::each takes an lv::list and the member of its rows that holds the key: lv::each(tracks, &track::id, track_row).")
lvgl26_compile_fail(each_key_of_another_row "lv::each takes an lv::list and the member of its rows that holds the key")
lvgl26_compile_fail_message(each_twice_in_one_widget static_assert "lv: a widget holds at most one lv::each. Put each one in a widget of its own: lv::column[lv::each(a, &row::id)[...]], lv::column[lv::each(b, &row::id)[...]].")
lvgl26_compile_fail_message(row_field_outside_each static_assert "lv: lv::bind(&T::field) follows a field of a row, and this node is not in a row. Use it in the row template of lv::each: lv::each(tracks, &track::id)[lv::label({.text = lv::bind(&track::title)})].")
lvgl26_compile_fail_message(row_field_plain_int static_assert "lv: a field of a plain integer does not say what the number is. If it is a measure, give it a unit with mp-units. If it counts things, use lv::counter. For anything else, name it: `struct pages { int value; };`. To allow plain integers, set rules.raw_integer_state in the settings.")
lvgl26_compile_fail_message(row_field_string_view static_assert "lv: a std::string_view field has no ending zero, which LVGL needs. Make the field a const char*, a std::string or a char array.")
lvgl26_compile_fail(row_field_not_a_value "a field of a row follows like a state")
lvgl26_compile_fail_message(row_field_function deleted "lv: lv::bind(&T::member) follows a data member of a row. A member function has no value to show: keep the value in a field of the row.")
lvgl26_compile_fail_message(row_field_wrong_property static_assert "lv: this property takes a whole number (not bool, not floating point), then true or false, and the state holds const char*. A state follows a property when its type, or the number inside its quantity or named type, is what the property takes.")
lvgl26_compile_fail(row_field_two_values "takes more than one value and cannot follow a field of a row")
lvgl26_compile_fail_message(row_view_unknown_id constexpr "lv: the row template has no id 'titel'. Its ids: title.")
lvgl26_compile_fail_message(row_template_duplicate_id constexpr "lv: the id 'a' is used twice in one row template. Ids must be unique.")
lvgl26_compile_fail(part_repeat_capturing_lambda "a_part_captures_nothing__name_what_it_needs_as_a_template_argument")
lvgl26_compile_fail_message(for_each_unknown_but static_assert "lv: lv::but names a value that is no enumerator of the enum. Name enumerators of the enum given to lv::for_each: lv::for_each<subpage, lv::but<subpage::none>>.")
lvgl26_compile_fail_message(for_each_part_rejects static_assert "lv: the part of lv::for_each does not accept this element as a template argument. Write it as a generic lambda that takes it: []<subpage Page> { ... } for an enumerator, []<lv::field F> { ... } for a member, []<record R> { ... } for a record.")
lvgl26_compile_fail_message(for_each_id_in_part constexpr "lv: a part of lv::for_each sets the id 'tile', and a repetition would use it more than once. Leave the id out: the repeated widgets are not reached by id. For rows that change at run time use lv::each.")
lvgl26_compile_fail_message(for_each_nothing_left constexpr "lv: lv::for_each<subpage> has no enumerator left to repeat. Every enumerator is left out by lv::but, or the enum has none.")
lvgl26_compile_fail_message(for_each_not_an_enum static_assert "lv: lv::for_each<E> takes an enum, one part for each of its enumerators. For a constant array write lv::for_each<items>(part), for the members of a struct lv::for_each_field<T>(part).")
lvgl26_compile_fail_message(for_each_part_not_a_widget static_assert "lv: the part of lv::for_each returns a widget: lv::button({...})[...], lv::row[...], lv::label({...}). It returned something else.")
lvgl26_compile_fail_message(for_each_field_without_caption constexpr "lv: lv::for_each_field<plain> finds no member with a caption, and a member without one is not repeated. Mark the members to show: [[=lv::caption{\"queue.owner\"}]] const char* owner;")
lvgl26_compile_fail_message(for_each_empty_array static_assert "lv: lv::for_each<items> has no record to repeat: the array is empty.")
lvgl26_compile_fail(caption_not_a_literal "a caption is a string literal")
lvgl26_compile_fail(like_missing_member "finds no member named 'scan'")
lvgl26_compile_fail_message(each_selected_wrong_type static_assert "lv: .selected follows the key of the selected row. Bind a state of the key's type: `constinit lv::state<track_id> current{};`, then .selected = lv::bind(current).")
lvgl26_compile_fail_message(each_selected_and_checked static_assert "lv: .selected gives the selected row the checked state, and this row template sets .checked itself. Keep one of them.")
lvgl26_compile_fail(props_without_braces "made_with_braces")
# The guard of include_only fails on an eager scan of the global namespace.
lvgl26_compile_fail(eager_scan "evaluation operation count exceeds limit of ${lvgl26_include_only_ops}")
target_compile_options(fail_eager_scan PRIVATE -fconstexpr-ops-limit=${lvgl26_include_only_ops})
lvgl26_compile_fail_message(set_inline_style deleted "lv: a style in set() is made with lv::style({...}): view.set({.style = lv::style({.radius = ...})}).")
lvgl26_compile_fail_message(set_named_props deleted "lv: pass the properties in place: view.set({...}). A prepared set of properties is a function that calls set().")
lvgl26_compile_fail_message(set_named_set_props deleted "lv: pass the properties in place: view.set({...}). A prepared set of properties is a function that calls set().")
lvgl26_compile_fail_message(set_constexpr_props deleted "lv: pass the properties in place: view.set({...}). A prepared set of properties is a function that calls set().")
lvgl26_compile_fail(props_unknown_name "no match for call to .*factory<lv::detail::fixed<4>.\"arc\"")
lvgl26_compile_fail_message(props_needs_unit static_assert "lv: wrong value for this property. It takes a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.")
lvgl26_compile_fail_message(props_wrong_unit static_assert "lv: wrong value for this property. It takes an angle in whole degrees (90 * lv::deg). You gave lv::length.")
lvgl26_compile_fail(props_wrong_order "designator order for field .*::id. does not match declaration order")
lvgl26_compile_fail_message(props_bool_as_int static_assert "lv: wrong value for this property. It takes a whole number (not bool, not floating point). You gave bool.")
lvgl26_compile_fail_message(props_narrowing static_assert "lv: wrong value for this property. It takes a whole number (not bool, not floating point). You gave double.")
lvgl26_compile_fail(units_negative_time "contract predicate is false in constant expression")
lvgl26_compile_fail(units_seconds_negative "contract predicate is false in constant expression")
lvgl26_compile_fail(units_seconds_too_long "contract predicate is false in constant expression")
lvgl26_compile_fail(units_rgb_too_large "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_not_whole "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_not_whole_negative "contract predicate is false in constant expression")
lvgl26_compile_fail(props_out_of_range "contract predicate is false in constant expression")
lvgl26_compile_fail(props_negative_unsigned "contract predicate is false in constant expression")
lvgl26_compile_fail(props_negative_unsigned_folded "contract predicate is false in constant expression")
# The same mistakes inside a larger expression. GCC 16.2 folds such a call without its
# contracts, so each of these compiled before lv/detail/checked.hpp.
lvgl26_compile_fail(units_negative_time_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_seconds_negative_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_seconds_too_long_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_rgb_too_large_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_count_too_large "contract predicate is false in constant expression")
lvgl26_compile_fail(units_count_too_large_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_not_whole_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(values_strict_out_of_range_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(match_not_an_enumerator "contract predicate is false in constant expression")
lvgl26_compile_fail(match_not_an_enumerator_folded "contract predicate is false in constant expression")
lvgl26_compile_fail_message(style_needs_token static_assert "lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a length (120 * lv::px, 50 * lv::pct or lv::size_content). You gave lv::length. A bare value is no token: write lv::raw(...) around it, or take it from the token file.")
lvgl26_compile_fail_message(style_raw_color static_assert "lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a color (lv::rgb(0x1b2430)). You gave a color. A bare value is no token: write lv::raw(...) around it, or take it from the token file.")
lvgl26_compile_fail_message(theme_needs_token static_assert "lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a length (120 * lv::px, 50 * lv::pct or lv::size_content). You gave lv::length. A bare value is no token: write lv::raw(...) around it, or take it from the token file.")
lvgl26_compile_fail_message(theme_not_a_value constexpr "lv: a theme or an lv::on cannot hold the style property 'enabled': LVGL keeps no value for it in a style. Leave it out, or set it in the node's own style.")
lvgl26_compile_fail(encoded_image_not_a_picture "lv::encoded_image takes a PNG or a JPEG file, and this is neither")
lvgl26_compile_fail(encoded_image_progressive "this JPEG file is progressive, or of another form than baseline")
lvgl26_compile_fail(encoded_image_empty "this picture is 0 x 0 pixels. LVGL holds a picture of 1 to 65535 pixels each way")
lvgl26_compile_fail(encoded_image_cut "this JPEG file is damaged: it does not say how large its picture is")
lvgl26_compile_fail(encoded_image_needs_png "LVGL is built without its PNG decoder. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_PNG ON\\)")
target_compile_options(fail_encoded_image_needs_png PRIVATE -ULV_USE_LODEPNG)  # as a build without the option
lvgl26_compile_fail(encoded_image_needs_jpeg "LVGL is built without its JPEG decoder. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_JPEG ON\\)")
target_compile_options(fail_encoded_image_needs_jpeg PRIVATE -ULV_USE_TJPGD)
lvgl26_compile_fail(image_not_a_png "this is not a PNG file. Embed one")
lvgl26_compile_fail(mask_not_a_png "this is not a PNG file. Embed one")
lvgl26_compile_fail(image_too_large "this picture is 200 x 100 pixels, and the compiler decodes at most 16384 .*raise limits.compile_time_image in the settings")
lvgl26_compile_fail(image_row_too_long "this picture is 16384 x 1 pixels, with rows of 65536 bytes. LVGL holds a picture of at most 65535")
lvgl26_compile_fail(image_too_wide "this picture is 70000 x 1 pixels, with rows of [0-9]+ bytes. LVGL holds a picture of at most 65535")
lvgl26_compile_fail(image_deep "this PNG file has 16 bits or fewer than 8 bits for each channel, or is interlaced")
lvgl26_compile_fail(image_interlaced "this PNG file has 16 bits or fewer than 8 bits for each channel, or is interlaced")
lvgl26_compile_fail(image_short "this PNG file is damaged: it does not hold as many pixels as it says")
lvgl26_compile_fail(image_bad_filter "this PNG file is damaged: a row has a filter that the format does not know")
lvgl26_compile_fail(scaled_font_needs_ttf "lv::scaled_font needs LVGL's TinyTTF, which is off. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_TTF ON\\)")
target_compile_options(fail_scaled_font_needs_ttf PRIVATE -ULV_USE_TINY_TTF)  # as a build without the option
lvgl26_compile_fail(scaled_font_raw_needs_ttf "lv::scaled_font needs LVGL's TinyTTF, which is off")
target_compile_options(fail_scaled_font_raw_needs_ttf PRIVATE -ULV_USE_TINY_TTF)
lvgl26_compile_fail(scaled_font_plain_size "the size of a font comes from a typography token")
lvgl26_compile_fail(scaled_font_bound_to_number "lv::bind\\(font\\) is for a font property of a style")
lvgl26_compile_fail_message(theme_bound_font constexpr "lv: a theme and an lv::on hold constants, and a font bound with lv::bind is drawn on the device. Give them a font made by the compiler (lv::typeface), or bind the font in the widget's own style.")
lvgl26_compile_fail_message(on_unknown_part constexpr "lv: widgets have no part 'knop'. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first.")
lvgl26_compile_fail_message(on_state_as_part constexpr "lv: 'pressed' is a state. lv::on takes the part first, then the state: lv::on(\"knob\", \"pressed\", {...}).")
lvgl26_compile_fail_message(on_unknown_state constexpr "lv: widgets have no state 'presed'. The states: default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4.")
lvgl26_compile_fail_message(on_part_as_state constexpr "lv: 'indicator' is a part. lv::on takes the part first, then the state: lv::on(\"indicator\", \"pressed\", {...}).")
lvgl26_compile_fail_message(on_unknown_name constexpr "lv: 'knop' is neither a part nor a state. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first. The states: default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4.")
lvgl26_compile_fail_message(on_any constexpr "lv: 'any' is neither a part nor a state. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first. The states: default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4.")
lvgl26_compile_fail_message(on_inside_on constexpr "lv: an lv::on cannot hold another lv::on. Put them side by side: base | lv::on(\"knob\", {...}) | lv::on(\"knob\", \"pressed\", {...}).")
lvgl26_compile_fail_message(on_not_a_value constexpr "lv: a theme or an lv::on cannot hold the style property 'enabled': LVGL keeps no value for it in a style. Leave it out, or set it in the node's own style.")
lvgl26_compile_fail_message(on_bound_font constexpr "lv: a theme and an lv::on hold constants, and a font bound with lv::bind is drawn on the device. Give them a font made by the compiler (lv::typeface), or bind the font in the widget's own style.")
lvgl26_compile_fail_message(on_in_part_slot static_assert "lv: lv::on goes in .style, because it names its own part and state. Write .style = base | lv::on(\"knob\", \"pressed\", {...}) instead of .style_knob = lv::on(...).")
lvgl26_compile_fail_message(theme_on_in_part_slot constexpr "lv: lv::on goes in .style, because it names its own part and state. Write .style = base | lv::on(\"knob\", \"pressed\", {...}) instead of .style_knob = lv::on(...).")
lvgl26_compile_fail(font_needs_token "It takes a font from a typography token .*or lv::raw\\(font\\) for a font that no token names")
lvgl26_compile_fail(font_not_a_font "this is not a TrueType font file. Embed a .ttf file")
lvgl26_compile_fail(font_no_outlines "this font has no TrueType outlines")
lvgl26_compile_fail(font_lacks_table "this font file lacks a table that lvgl26 reads")
lvgl26_compile_fail(font_size_out_of_range "a font size is from 4 px to 255 px")
lvgl26_compile_fail(font_no_glyph "the font file has no glyph for the character U\\+0416")
lvgl26_compile_fail(font_ascii_plus_no_glyph "the font file has no glyph for the character U\\+0416")
lvgl26_compile_fail_message(font_ascii_plus_not_utf8 constexpr "lv: lv::ascii_plus takes UTF-8 text, and byte 1 of it is not part of a UTF-8 character.")
lvgl26_compile_fail_message(font_ascii_plus_too_many constexpr "lv: a font holds at most 255 characters, and the 95 of the default set and the 161 you add are 256. Make a second font for the others.")
lvgl26_compile_fail_message(font_latin1_plus_not_utf8 constexpr "lv: lv::latin1_plus takes UTF-8 text, and byte 1 of it is not part of a UTF-8 character.")
lvgl26_compile_fail_message(font_latin1_plus_too_many constexpr "lv: a font holds at most 255 characters, and the 191 of lv::latin1 and the 65 you add are 256. Make a second font for the others.")
lvgl26_compile_fail(font_glyph_too_large "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_glyph_too_tall "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_too_many_characters "a font holds at most 255 characters")
lvgl26_compile_fail(font_bitmap_too_large "the pixels of this font take 1538941 bytes. LVGL's font format holds at most 1048575")
lvgl26_compile_fail_message(typeface_no_file constexpr "lv: the typeface has no file for the family 'Montserrat' at weight 700. Add it: lv::typeface({{\"Montserrat\", 700, its_ttf}}).")
lvgl26_compile_fail_message(typeface_too_many_files constexpr "lv: a typeface has at most eight files.")
lvgl26_compile_fail(signal_numerals_not_a_numeral "the typeface has no file for the family 'Barlow' at weight 500. Add it")
lvgl26_compile_fail_message(style_wrong_token static_assert "lv: wrong value for this property. It takes a design token (for example tok.space.md) or lv::raw(...), holding a length (120 * lv::px, 50 * lv::pct or lv::size_content). You gave lv::token<lv_color_t>.")
lvgl26_compile_fail_message(token_bad_alias constexpr "lv: design tokens: 'color.primary': the alias {color.brand} points at nothing.")
lvgl26_compile_fail_message(token_bad_color constexpr "lv: design tokens: 'color.primary': the color '#33zz66' has the character 'z', which is not a hex digit. Write it as #rgb or #rrggbb.")
lvgl26_compile_fail_message(token_color_alpha constexpr "lv: design tokens: 'card.shadow': the color '#01020380' has an alpha channel, which a color token does not carry. Write it as #rrggbb and give the opacity its own token.")
# The forms from before the Design Tokens Format 2025.10 are errors unless rules.tokens_legacy_values is set.
lvgl26_compile_fail_message(token_legacy_color constexpr "lv: design tokens: 'c.ink' is written \"#3366ff\", a form from before the Design Tokens Format 2025.10. The standard writes it as {\"colorSpace\": \"srgb\", \"components\": [0.2, 0.4, 1], \"hex\": \"#3366ff\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_legacy_dimension constexpr "lv: design tokens: 's.gap' is written \"8px\", a form from before the Design Tokens Format 2025.10. The standard writes it as {\"value\": 8, \"unit\": \"px\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_legacy_duration constexpr "lv: design tokens: 't.fast' is written \"200ms\", a form from before the Design Tokens Format 2025.10. The standard writes it as {\"value\": 200, \"unit\": \"ms\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_legacy_number constexpr "lv: design tokens: 's.gap' is written 8, a form from before the Design Tokens Format 2025.10. The standard writes it as {\"value\": 8, \"unit\": \"px\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_legacy_member constexpr "lv: design tokens: 't.body' member 'fontSize' is written \"16px\", a form from before the Design Tokens Format 2025.10. The standard writes it as {\"value\": 16, \"unit\": \"px\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_legacy_in_list constexpr "lv: design tokens: 's.lift' member 'offsetX' is written \"0px\", a form from before the Design Tokens Format 2025.10. The standard writes it as {\"value\": 0, \"unit\": \"px\"}. Run python3 tools/dtcg_fix.py on the token file, or set rules.tokens_legacy_values in the settings while the files convert")
lvgl26_compile_fail_message(token_color_no_hash constexpr "lv: design tokens: 'color.brand': the color 'x123456' is not #rgb or #rrggbb.")
lvgl26_compile_fail_message(token_color_too_long constexpr "lv: design tokens: 'color.brand': the color '#1122334455' is not #rgb or #rrggbb.")
lvgl26_compile_fail_message(token_color_too_short constexpr "lv: design tokens: 'color.brand': the color '#' is not #rgb or #rrggbb.")
lvgl26_compile_fail_message(token_color_five_digits constexpr "lv: design tokens: 'color.brand': the color '#12345' is not #rgb or #rrggbb.")
lvgl26_compile_fail_message(token_color_two_components constexpr "lv: design tokens: 'color.brand': a color needs a hex string or three components, and this one has 2.")
lvgl26_compile_fail_message(token_duration_unit_h constexpr "lv: design tokens: 'time.fast': the duration unit 'h' is not ms or s.")
lvgl26_compile_fail_message(token_duration_unit_us constexpr "lv: design tokens: 'time.fast': the duration unit 'us' is not ms or s.")
lvgl26_compile_fail_message(token_length_unit_bang constexpr "lv: design tokens: 'space.gap': the dimension unit '!' is not px, rem or %.")
lvgl26_compile_fail_message(token_unit_without_number constexpr "lv: design tokens: 'space.gap': 'px' has no number before its unit.")
lvgl26_compile_fail_message(token_bad_weight constexpr "lv: design tokens: 'type.weight': the font weight 'thin' is not a number, normal or bold.")
lvgl26_compile_fail_message(token_gradient_not_a_list constexpr "lv: design tokens: 'fill.sky': a gradient token must be a list of at most eight stops, and this one is not a list.")
lvgl26_compile_fail_message(token_alias_too_deep constexpr "lv: design tokens: 'g.t8': the alias {g.t9} follows more than 8 aliases in a row, the most lvgl26 follows. Point the first alias of the chain nearer its value.")
lvgl26_compile_fail_message(token_alias_circle constexpr "lv: design tokens: 'g.a': the alias {g.b} goes in a circle.")
lvgl26_compile_fail(token_value_at_end "contract predicate is false")
lvgl26_compile_fail_message(token_bezier_three_numbers constexpr "lv: design tokens: 'ease.out': a cubicBezier token must be a list of four numbers, and this one has 3.")
lvgl26_compile_fail_message(token_bezier_not_a_list constexpr "lv: design tokens: 'ease.out': a cubicBezier token must be a list of four numbers, and this one is not a list.")
lvgl26_compile_fail_message(token_gradient_nine_stops constexpr "lv: design tokens: 'fill.sky': a gradient token must be a list of at most eight stops, and this one has 9.")
lvgl26_compile_fail(token_value_past_end "contract predicate is false")
lvgl26_compile_fail_message(token_bad_unit constexpr "lv: design tokens: 'space.md': the dimension unit 'pt' is not px, rem or %.")
lvgl26_compile_fail(token_misspelled "has no member named 'primry'")
lvgl26_compile_fail_message(token_bad_shadow constexpr "lv: design tokens: 'card.shadow': this composite token has no 'blur'. Add it.")
lvgl26_compile_fail(units_decidegrees_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_number_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_number_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_too_large "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_too_large_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_decidegrees_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_decidegrees_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_fraction_for_whole_angle "this takes an angle in whole degrees, and the value may have a fraction of a degree")
lvgl26_compile_fail_message(props_fraction_for_whole_angle static_assert "lv: wrong value for this property. It takes an angle in whole degrees (90 * lv::deg). You gave lv::decidegrees.")
lvgl26_compile_fail_message(bind_angle_to_decidegrees static_assert "lv: this property is in tenths of a degree, and a state of lv::angle holds whole degrees. Give the state the type of the property: `lv::state<lv::decidegrees> turn{90 * lv::deg};`.")
lvgl26_compile_fail(angle_header_not_read "'lv_dial_set_rotation' takes or returns an angle, and only LVGL's doc comment says .*angle_header_not_read.cpp. Add that header to angle_headers")
lvgl26_compile_fail(angle_mark_not_found "no doc comment in LVGL's headers says '0.1 degree' any more")
lvgl26_compile_fail_message(bind_wrong_type static_assert "lv: this property takes a whole number (not bool, not floating point), and the state holds bool. A state follows a property when its type, or the number inside its quantity or named type, is what the property takes.")
lvgl26_compile_fail_message(bind_format_not_text static_assert "lv: lv::bind(state, format) is for text properties only, and this property takes a whole number (not bool, not floating point). Bind the state without a format: lv::bind(state).")
lvgl26_compile_fail_message(state_bad_type static_assert "lv: a state holds a quantity (`0 * km / h`, with mp-units), a struct with one integer, bool or enum member, a bool, an enum, an lv_color_t or a pointer (for text: const char*). This state's type is double.")
lvgl26_compile_fail_message(bind_two_values static_assert "lv: this property takes a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length), then a height or y (120 * lv::px, 50 * lv::pct or lv::size_content, not a horizontal length), and the state holds offset. A state follows a property when its type, or the number inside its quantity or named type, is what the property takes.")
lvgl26_compile_fail(config_too_late "specialization of .lv::cfg<lv::override>. after instantiation")
# Each part of lvgl26 that a project can forbid, with the message that names the setting.
lvgl26_compile_fail(restrict_ui_scheduler "lv: lv::ui_scheduler needs threads, and this project does not allow them \\(allow\\.threads\\)\\.")
lvgl26_compile_fail(restrict_post "lv: lv::ui_scheduler::post needs allocation, and this project does not allow it \\(allow\\.allocation\\)\\. It keeps each piece of posted work on the heap until the UI thread runs it\\.")
lvgl26_compile_fail(restrict_encode_png "lv: lv::encode_png needs allocation, and this project does not allow it \\(allow\\.allocation\\)\\. The bytes of the file are a std::vector\\.")
lvgl26_compile_fail(restrict_write_png "lv: lv::write_png needs files, and this project does not allow them \\(allow\\.files\\)\\.")
lvgl26_compile_fail(restrict_mcp_server "lv: lv::mcp::server needs allocation, and this project does not allow it \\(allow\\.allocation\\)\\. Its answers are std::string\\.")
lvgl26_compile_fail(restrict_mcp_serve "lv: lv::mcp::server::serve needs files, and this project does not allow them \\(allow\\.files\\)\\. It reads and writes standard streams\\.")
lvgl26_compile_fail_message(restrict_runtime static_assert "lv: lv::runtime needs floating point, and this project does not allow it (allow.floating_point). It reads the numbers of a document as double.")
lvgl26_compile_fail_message(restrict_spawn static_assert "lv: lv::spawn needs allocation, and this project does not allow it (allow.allocation). A task's frame is taken while the program runs, in a number the compiler does not know; until tasks get a fixed region of memory of their own, a program without allocation runs no task.")
lvgl26_compile_fail_message(restrict_spawn_owned static_assert "lv: lv::spawn needs allocation, and this project does not allow it (allow.allocation). A task's frame is taken while the program runs, in a number the compiler does not know; until tasks get a fixed region of memory of their own, a program without allocation runs no task.")
lvgl26_compile_fail(restrict_from_file "lv: lv::ui_scheduler needs threads, and this project does not allow them \\(allow.threads in settings/restricted.json\\).")
lvgl26_settings_header(${CMAKE_CURRENT_SOURCE_DIR}/settings/restricted.json settings/restricted.json
                       ${CMAKE_CURRENT_BINARY_DIR}/lvgl26-settings/restricted restricted_hash)
target_compile_options(fail_restrict_from_file PRIVATE -include ${CMAKE_CURRENT_BINARY_DIR}/lvgl26-settings/restricted/settings.hpp)
# Each mistake in a settings file, with the message that names it.
# A settings file and a specialization of lv::cfg at once: the compiler shows the line of the
# file's header that says what to do.
lvgl26_compile_fail(settings_both_forms "lv: the settings of this project are in settings/project.json\\; use the file or a specialization of lv::cfg, not both")
lvgl26_settings_header(${CMAKE_CURRENT_SOURCE_DIR}/settings/project.json settings/project.json
                       ${CMAKE_CURRENT_BINARY_DIR}/lvgl26-settings/fail both_forms_hash)
target_compile_options(fail_settings_both_forms PRIVATE -include ${CMAKE_CURRENT_BINARY_DIR}/lvgl26-settings/fail/settings.hpp)
lvgl26_compile_fail_message(settings_unknown_key static_assert "lv: test.json: unknown key 'thread' in allow. Did you mean 'threads'? The keys of allow: allocation, threads, files, floating_point.")
lvgl26_compile_fail_message(settings_unknown_section static_assert "lv: test.json: unknown key 'alow'. Did you mean 'allow'? The keys of the file: allow, rules, limits.")
lvgl26_compile_fail_message(settings_not_bool static_assert "lv: test.json: allow.threads takes true or false, not \"no\".")
lvgl26_compile_fail_message(settings_section_not_object static_assert "lv: test.json: rules takes an object of settings, not true. Its keys: raw_style_values, raw_integer_state, tokens_legacy_values.")
lvgl26_compile_fail_message(settings_not_json static_assert "lv: test.json: the file is not well-formed JSON. At line 1, column 12 it expects a key in quotes or '}', and the file ends there.")
lvgl26_compile_fail_message(settings_not_object static_assert "lv: test.json: the file takes an object of settings, not [1, 2]. Its keys: allow, rules, limits.")
lvgl26_compile_fail_message(settings_amount_plain static_assert "lv: test.json: limits.compile_time_image takes a value and a unit, as {\"value\": 4, \"unit\": \"px\"}, not 16384. Its units: px.")
lvgl26_compile_fail_message(settings_unknown_unit static_assert "lv: test.json: limits.compile_time_image has the unit 'pix'. Did you mean 'px'? Its units: px.")
lvgl26_compile_fail_message(settings_negative static_assert "lv: test.json: limits.compile_time_image is -1 px, and a limit cannot be negative.")
lvgl26_compile_fail_message(settings_too_large static_assert "lv: test.json: limits.compile_time_image is 1e300 px, more than a limit can hold.")
lvgl26_compile_fail_message(settings_fraction static_assert "lv: test.json: limits.compile_time_image is 0.5 px, which is no whole number of px.")
lvgl26_compile_fail_message(state_plain_int static_assert "lv: a state of a plain integer does not say what the number is. If it is a measure, give it a unit with mp-units: `lv::state speed{0 * km / h};`. If it counts things, use lv::counter: `lv::state clicks{lv::counter{}};`. For anything else, name it: `struct tab_index { int value; };`. To allow plain integers, set rules.raw_integer_state in the settings.")
lvgl26_compile_fail(units_wrong_axis "It takes a width or x .*not a vertical length")
lvgl26_compile_fail(units_bool_count "no match for 'operator\\*'")
lvgl26_compile_fail_message(bind_printf_format constexpr "lv: a text binding format needs exactly one {} for the value, for example \"{} km/h\".")
lvgl26_compile_fail(quantity_wrong_dimension "no match for .operator=")
lvgl26_compile_fail(quantity_lossy "no match for .operator=")
lvgl26_compile_fail(quantity_floating "a quantity in a state needs an integer number for now")
lvgl26_compile_fail_message(computed_plain_int static_assert "lv: the function of an lv::computed returns a plain integer, which does not say what the number is. Return a quantity (the difference of two speeds is one), an lv::counter, or a named type: `struct tab_index { int value; };`. To allow plain integers, set rules.raw_integer_state in the settings.")
lvgl26_compile_fail_message(computed_bad_result static_assert "lv: the function of an lv::computed returns double, which a state cannot hold. Return a quantity with an integer number (`s - l`, with mp-units), a struct with one integer, bool or enum member, a bool, an enum, an lv_color_t or a pointer (for text: const char*). For a side effect, use lv::effect.")
lvgl26_compile_fail_message(computed_wrong_arguments static_assert "lv: the function of an lv::computed takes the values of its dependencies, in their order: (lv::counter, bool). Take each by value or by const reference. It is called as const: a `mutable` lambda does not fit.")
lvgl26_compile_fail_message(computed_not_a_state static_assert "lv: a dependency of an lv::computed must be an lv::state or an lv::computed that outlives it. Pass the object itself, as in `lv::computed{f, speed}`: not its value (`speed.get()`) and not a temporary.")
lvgl26_compile_fail_message(computed_temporary static_assert "lv: a dependency of an lv::computed must be an lv::state or an lv::computed that outlives it. Pass the object itself, as in `lv::computed{f, speed}`: not its value (`speed.get()`) and not a temporary.")
lvgl26_compile_fail_message(computed_constexpr constexpr "lv: an lv::computed cannot be a constexpr variable: binding it writes to it. Write `constinit lv::computed`, as for a state.")
lvgl26_compile_fail_message(effect_wrong_arguments static_assert "lv: the function of an lv::effect takes the values of its dependencies, in their order: (lv::counter). Take each by value or by const reference.")
lvgl26_compile_fail_message(effect_not_a_state static_assert "lv: a dependency of an lv::effect must be an lv::state or an lv::computed that outlives it. Pass the object itself, as in `lv::effect{f, speed}`: not its value (`speed.get()`) and not a temporary.")
lvgl26_compile_fail_message(bind_computed_wrong_type static_assert "lv: this property takes a whole number (not bool, not floating point), and the state holds bool. A state follows a property when its type, or the number inside its quantity or named type, is what the property takes.")
lvgl26_compile_fail(first_mixed_handlers "in lv::first, give a handler to every item")
lvgl26_compile_fail(animate_not_a_number "no matching function for call to .animate.lv::view<lv::detail::fixed<6>.\"label\"}>&, <brace-enclosed initializer list>")
lvgl26_compile_fail(animate_needs_unit "It takes a width or x .*You gave int")
lvgl26_compile_fail(animate_one_end "an animation needs both ends: .property = {from, to}")
lvgl26_compile_fail(animate_mixed_units "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_mixed_units_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_size_content "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_then_bad_handler "no matching function for call to .*then")
# lv::in names the part and the state of a style animation (include/lv/animate.hpp).
lvgl26_compile_fail_message(in_three_names static_assert "lv: lv::in takes a part and a state at most: lv::in<\"knob\", \"pressed\">(view).")
lvgl26_compile_fail_message(in_state_first constexpr "lv: 'pressed' is a state. lv::in takes the part first, then the state: lv::in<\"knob\", \"pressed\">(view).")
lvgl26_compile_fail_message(in_part_second constexpr "lv: 'indicator' is a part. lv::in takes the part first, then the state: lv::in<\"indicator\", \"pressed\">(view).")
lvgl26_compile_fail_message(in_unknown_part constexpr "lv: widgets have no part 'knop'. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first.")
lvgl26_compile_fail_message(in_unknown_state constexpr "lv: widgets have no state 'presed'. The states: default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4.")
lvgl26_compile_fail_message(in_unknown_name constexpr "lv: 'pressd' is neither a part nor a state. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first. The states: default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4.")
lvgl26_compile_fail(in_widget_setter "no matching function for call to .animate")
# display.follow takes the state of an enum.
lvgl26_compile_fail(follow_not_an_enum "no matching function for call to .*follow|constraints not satisfied")
lvgl26_compile_fail(animate_style_plain_number "It takes an opacity .50 \\* lv::pct.. You gave int")
lvgl26_compile_fail(post_not_callable "no matching function for call to .lv::ui_scheduler::post")
lvgl26_compile_fail_message(match_missing_case static_assert "lv::match does not cover: sport. Add an arm for each, or a last arm `lv::otherwise.then(...)`.")
lvgl26_compile_fail_message(match_duplicate_case static_assert "lv::match has two arms for eco. Each value needs exactly one.")
lvgl26_compile_fail_message(match_other_enum static_assert "lv::match on page has an arm with a value of look. Name a value of page: lv::is<page::...>.")
lvgl26_compile_fail_message(match_redundant_otherwise static_assert "lv::match already covers every value, so its lv::otherwise arm can never match. Remove it.")
lvgl26_compile_fail_message(match_needs_otherwise static_assert "lv::match on this type cannot list every value, so it needs a last arm `lv::otherwise.then(...)`.")
lvgl26_compile_fail_message(match_two_otherwise static_assert "lv::match has more than one lv::otherwise arm.")
lvgl26_compile_fail_message(match_otherwise_not_last static_assert "lv::match: lv::otherwise must be the last arm; the arms after it can never match.")
lvgl26_compile_fail_message(match_not_an_arm deleted "lv::match takes arms: lv::is<...>.then(handler) or lv::otherwise.then(handler). A handler takes the matched value or nothing.")
lvgl26_compile_fail_message(match_bad_handler deleted "lv::match takes arms: lv::is<...>.then(handler) or lv::otherwise.then(handler). A handler takes the matched value or nothing.")
lvgl26_compile_fail_message(match_stored deleted "lv::match(value)(arms...) must be one expression: the matcher refers to the value and must not be stored.")
lvgl26_compile_fail(match_not_comparable "no matching function for call to .match.blob")
lvgl26_compile_fail(set_wrong_argument "no matching function for call to .*set<.value.>")
lvgl26_compile_fail(then_bad_handler "no matching function for call to .*then")
lvgl26_compile_fail(weak_not_a_view "requires +view_value<V> class lv::weak")
lvgl26_compile_fail(owned_not_a_view "requires +view_value<V> class lv::owned")
lvgl26_compile_fail_message(enumerator_unknown_name constexpr "lv: align has no value 'centre'. Its values: default, top_left, top_mid, top_right, bottom_left, bottom_mid, bottom_right, left_mid, right_mid, center, out_top_left, out_top_mid, out_top_right, out_bottom_left, out_bottom_mid, out_bottom_right, out_left_top, out_left_mid, out_left_bottom, out_right_top, out_right_mid, out_right_bottom.")
lvgl26_compile_fail_message(enumerator_unknown_name_beside_text constexpr "lv: roller_mode has no value 'endless'. Its values: normal, infinite.")
lvgl26_compile_fail(mount_not_a_tree "no matching function for call to .mount<not_a_tree>")
lvgl26_compile_fail(mounted_not_a_tree "template constraint failure for .*class lv::mounted")
lvgl26_compile_fail(place_not_a_tree "no matching function for call to .place<not_a_tree>")
lvgl26_compile_fail(row_view_not_a_tree "template constraint failure for .*class lv::row_view")
lvgl26_compile_fail(frame_without_size "lv::frame<Width, Height>: both must be greater than 0")
lvgl26_compile_fail(band_without_screen "lv::band<Width, Height, Lines>: the screen must be larger than 0")
lvgl26_compile_fail(band_taller_than_screen "Lines is the height of the band, from 1 to Height")
lvgl26_compile_fail(display_frame_temporary_panel "it is told which area changed, and the pixels are in the frame")
lvgl26_compile_fail(display_band_panel_without_pixels "it is given each piece of the screen")

# The quick start of the documentation, built the way its page tells a reader to: as
# a project of its own that fetches lvgl26. It compiles LVGL once more, so it runs
# where the build is plain: not under a sanitizer, the analyzer or coverage.
if(NOT LVGL26_SANITIZE AND NOT LVGL26_ANALYZE AND NOT LVGL26_COVERAGE)
  set(LVGL26_QUICK_START ${CMAKE_CURRENT_BINARY_DIR}/quick_start)
  add_test(NAME quick_start
           COMMAND ${CMAKE_CTEST_COMMAND} --build-and-test
                   ${PROJECT_SOURCE_DIR}/examples/quick_start ${LVGL26_QUICK_START}
                   --build-generator ${CMAKE_GENERATOR}
                   --build-options -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                   -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                   -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                   -DLVGL26_LVGL_REF=${LVGL26_LVGL_REF}
                                   -DFETCHCONTENT_SOURCE_DIR_LVGL26=${PROJECT_SOURCE_DIR}
                                   -DFETCHCONTENT_SOURCE_DIR_LVGL=${lvgl_SOURCE_DIR}
                                   -DFETCHCONTENT_SOURCE_DIR_MP-UNITS=${mp-units_SOURCE_DIR}
                   --test-command quick_start)
  set_tests_properties(quick_start PROPERTIES TIMEOUT 900 FIXTURES_SETUP quick_start_picture)
  # The picture on the page is the one the program writes. LVGL v10 may draw other pixels.
  if(LVGL26_LVGL_REF STREQUAL "v9.6.0")
    add_test(NAME quick_start_picture
             COMMAND ${CMAKE_COMMAND} -E compare_files ${LVGL26_QUICK_START}/quick_start.png
                     ${PROJECT_SOURCE_DIR}/docs/media/quick-start.png)
    set_tests_properties(quick_start_picture PROPERTIES FIXTURES_REQUIRED quick_start_picture)
  endif()
endif()

# The example of docs/concepts.md: parts that constrain their parameters with the public concepts.
add_executable(example_parts ${PROJECT_SOURCE_DIR}/examples/parts/main.cpp)
target_link_libraries(example_parts PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_parts COMMAND example_parts)

# The example of docs/options.md: the texts of an enum's options, and some enumerators left out.
add_executable(example_options ${PROJECT_SOURCE_DIR}/examples/options/main.cpp)
target_link_libraries(example_options PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_options COMMAND example_options)

# The example of docs/lists.md. Its picture is the one on the page.
add_executable(example_lists ${PROJECT_SOURCE_DIR}/examples/lists/main.cpp)
target_compile_options(example_lists PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/lists)
target_link_libraries(example_lists PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_lists_out)
add_test(NAME example_lists COMMAND example_lists WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_lists_out)
set_tests_properties(example_lists PROPERTIES FIXTURES_SETUP example_lists_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_lists_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_lists_out/lists.png
                   ${PROJECT_SOURCE_DIR}/docs/media/lists.png)
  set_tests_properties(example_lists_picture PROPERTIES FIXTURES_REQUIRED example_lists_picture)
endif()

# The example of docs/state.md: states, what follows them, and an effect.
add_executable(example_state ${PROJECT_SOURCE_DIR}/examples/state/main.cpp)
target_link_libraries(example_state PRIVATE lvgl26::lvgl26 lvgl26_warnings mp-units::mp-units)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_state_out)
add_test(NAME example_state COMMAND example_state WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_state_out)
set_tests_properties(example_state PROPERTIES FIXTURES_SETUP example_state_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_state_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_state_out/state.png
                   ${PROJECT_SOURCE_DIR}/docs/media/state.png)
  set_tests_properties(example_state_picture PROPERTIES FIXTURES_REQUIRED example_state_picture)
endif()

# The example of docs/repetition.md: it runs, and tests/for_each.cpp checks what it repeats.
add_executable(example_repeat ${PROJECT_SOURCE_DIR}/examples/repeat/main.cpp)
target_link_libraries(example_repeat PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_repeat COMMAND example_repeat)

# The example of docs/mounting.md: a tree in a variable, in a global, in memory set aside.
add_executable(example_mounting ${PROJECT_SOURCE_DIR}/examples/mounting/main.cpp)
target_link_libraries(example_mounting PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_mounting COMMAND example_mounting)

# Pages that follow a state: lv::shown and lv::when.
add_executable(example_pages ${PROJECT_SOURCE_DIR}/examples/pages/main.cpp)
target_link_libraries(example_pages PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_pages COMMAND example_pages)

# The example of docs/themes.md. Its pictures are the ones on the page.
add_executable(example_theme ${PROJECT_SOURCE_DIR}/examples/theme/main.cpp)
target_compile_options(example_theme PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/theme)
target_link_libraries(example_theme PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_theme_out)
add_test(NAME example_theme COMMAND example_theme WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_theme_out)
set_tests_properties(example_theme PROPERTIES FIXTURES_SETUP example_theme_pictures)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  foreach(look signal paper)
    add_test(NAME example_theme_picture_${look}
             COMMAND ${CMAKE_COMMAND} -E compare_files
                     ${CMAKE_CURRENT_BINARY_DIR}/example_theme_out/theme_${look}.png
                     ${PROJECT_SOURCE_DIR}/docs/media/theme-${look}.png)
    set_tests_properties(example_theme_picture_${look} PROPERTIES FIXTURES_REQUIRED example_theme_pictures)
  endforeach()
endif()

# The example of docs/looks.md. Its picture is the one on the page.
add_executable(example_looks ${PROJECT_SOURCE_DIR}/examples/looks/main.cpp)
target_compile_options(example_looks PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/looks
                       --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_looks PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_looks_out)
add_test(NAME example_looks COMMAND example_looks WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_looks_out)
set_tests_properties(example_looks PROPERTIES FIXTURES_SETUP example_looks_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_looks_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files
                   ${CMAKE_CURRENT_BINARY_DIR}/example_looks_out/looks.png
                   ${PROJECT_SOURCE_DIR}/docs/media/looks.png)
  set_tests_properties(example_looks_picture PROPERTIES FIXTURES_REQUIRED example_looks_picture)
endif()

# The bouncing ball of examples/bounce: its motion on a stepped clock, and pictures at fixed times.
add_executable(example_bounce example_bounce.cpp)
target_include_directories(example_bounce PRIVATE ${PROJECT_SOURCE_DIR}/examples/bounce)
target_compile_options(example_bounce PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/bounce)
target_link_libraries(example_bounce PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_bounce COMMAND example_bounce)
set(bounce_times 300 420 730 1500 3000)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_bounce_out)
add_test(NAME example_bounce_shot COMMAND bounce_shot ${CMAKE_CURRENT_BINARY_DIR}/example_bounce_out ${bounce_times})
set_tests_properties(example_bounce_shot PROPERTIES FIXTURES_SETUP example_bounce_pictures)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  foreach(at IN LISTS bounce_times)
    add_test(NAME example_bounce_picture_${at}
             COMMAND ${CMAKE_COMMAND} -E compare_files
                     ${CMAKE_CURRENT_BINARY_DIR}/example_bounce_out/bounce_${at}.png
                     ${CMAKE_CURRENT_SOURCE_DIR}/ref/bounce_${at}.png)
    set_tests_properties(example_bounce_picture_${at} PROPERTIES FIXTURES_REQUIRED example_bounce_pictures)
  endforeach()
endif()

# The same screen with looks and with every value written out: the same pixels.
add_executable(looks_example looks_example.cpp)
target_compile_options(looks_example PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/looks
                       --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(looks_example PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME looks_example COMMAND looks_example)

# The second example of docs/themes.md: Signal with its typefaces.
add_executable(example_signal_type ${PROJECT_SOURCE_DIR}/examples/signal_type/main.cpp)
target_link_libraries(example_signal_type PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_signal_type_out)
add_test(NAME example_signal_type COMMAND example_signal_type
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_signal_type_out)
set_tests_properties(example_signal_type PROPERTIES FIXTURES_SETUP example_signal_type_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_signal_type_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files
                   ${CMAKE_CURRENT_BINARY_DIR}/example_signal_type_out/signal_type.png
                   ${PROJECT_SOURCE_DIR}/docs/media/theme-signal-type.png)
  set_tests_properties(example_signal_type_picture PROPERTIES FIXTURES_REQUIRED example_signal_type_picture)
endif()

# The example of docs/images.md. Its picture is the one on the page.
add_executable(example_images ${PROJECT_SOURCE_DIR}/examples/images/main.cpp)
target_compile_options(example_images PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/images)
target_link_libraries(example_images PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_images_out)
add_test(NAME example_images COMMAND example_images WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_images_out)
set_tests_properties(example_images PROPERTIES FIXTURES_SETUP example_images_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_images_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_images_out/images.png
                   ${PROJECT_SOURCE_DIR}/docs/media/images.png)
  set_tests_properties(example_images_picture PROPERTIES FIXTURES_REQUIRED example_images_picture)
endif()

# The example of docs/icons.md. Its picture is the one on the page.
add_executable(example_icons ${PROJECT_SOURCE_DIR}/examples/icons/main.cpp)
target_compile_options(example_icons PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/icons
                                             --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_icons PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_icons_out)
add_test(NAME example_icons COMMAND example_icons WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_icons_out)
set_tests_properties(example_icons PROPERTIES FIXTURES_SETUP example_icons_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_icons_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_icons_out/icons.png
                   ${PROJECT_SOURCE_DIR}/docs/media/icons.png)
  set_tests_properties(example_icons_picture PROPERTIES FIXTURES_REQUIRED example_icons_picture)
endif()

# The second example of docs/fonts.md: a font drawn on the device.
add_executable(example_scaled_font ${PROJECT_SOURCE_DIR}/examples/scaled_font/main.cpp)
target_compile_options(example_scaled_font PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/fonts
                                                   --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_scaled_font PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_scaled_font COMMAND example_scaled_font)

# The example of docs/themes.md: a display that follows a state.
add_executable(example_theme_follow ${PROJECT_SOURCE_DIR}/examples/theme_follow/main.cpp)
target_link_libraries(example_theme_follow PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_theme_follow_out)
add_test(NAME example_theme_follow COMMAND example_theme_follow WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_theme_follow_out)

# The example of docs/fonts.md. Its picture is the one on the page.
add_executable(example_fonts ${PROJECT_SOURCE_DIR}/examples/fonts/main.cpp)
target_compile_options(example_fonts PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/fonts
                                             --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_fonts PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_out)
add_test(NAME example_fonts COMMAND example_fonts WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_out)
set_tests_properties(example_fonts PROPERTIES FIXTURES_SETUP example_fonts_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_fonts_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_out/fonts.png
                   ${PROJECT_SOURCE_DIR}/docs/media/fonts.png)
  set_tests_properties(example_fonts_picture PROPERTIES FIXTURES_REQUIRED example_fonts_picture)
endif()

# The same screen with each font drawn in a file of its own (docs/fonts.md): its picture must be
# the one of the example above, pixel for pixel.
set(fonts_split_dir ${PROJECT_SOURCE_DIR}/examples/fonts_split)
add_executable(example_fonts_split ${fonts_split_dir}/main.cpp ${fonts_split_dir}/font_text_label.cpp
                                   ${fonts_split_dir}/font_text_title.cpp ${fonts_split_dir}/font_text_numeral.cpp)
target_compile_options(example_fonts_split PRIVATE --embed-dir=${fonts_split_dir} --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_fonts_split PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_split_out)
add_test(NAME example_fonts_split COMMAND example_fonts_split WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_split_out)
set_tests_properties(example_fonts_split PROPERTIES FIXTURES_SETUP example_fonts_split_picture)
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")  # LVGL v10 may draw other pixels
  add_test(NAME example_fonts_split_picture
           COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/example_fonts_split_out/fonts_split.png
                   ${PROJECT_SOURCE_DIR}/docs/media/fonts.png)
  set_tests_properties(example_fonts_split_picture PROPERTIES FIXTURES_REQUIRED example_fonts_split_picture)
endif()

# The reference data: what reflection finds in the LVGL of this build, as JSON. The
# committed file must be what the program prints.
add_executable(lv_reference ${PROJECT_SOURCE_DIR}/tools/reference/reference.cpp)
target_link_libraries(lv_reference PRIVATE lvgl26::lvgl26 lvgl26_warnings)
# The document is one constant evaluation over every widget: 232 to 256 million steps with
# LVGL v9.6, 208 to 232 million with v10, where GCC's default is 33554432. This program
# alone gets a budget, at about twice its need, so that a change which makes it far more
# expensive still stops the build.
target_compile_options(lv_reference PRIVATE -fconstexpr-ops-limit=500000000)
# Not for the analyzer: this file is one long constant evaluation over every widget, and
# the analyzer on top of it needs more memory than a CI machine has. What runs is a loop
# that prints. A source file's options come last, after -fanalyzer.
set_source_files_properties(${PROJECT_SOURCE_DIR}/tools/reference/reference.cpp PROPERTIES COMPILE_OPTIONS -fno-analyzer)
# The v10 pin is a commit, which is no name for a file.
if(LVGL26_LVGL_REF MATCHES "^v[0-9.]+$")
  set(LVGL26_REFERENCE_FILE lvgl-${LVGL26_LVGL_REF}.json)
else()
  set(LVGL26_REFERENCE_FILE lvgl-v10-pin.json)
endif()
add_test(NAME reference_data
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lv_reference>
                 -DEXPECTED=${PROJECT_SOURCE_DIR}/docs/reference/${LVGL26_REFERENCE_FILE}
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_output.cmake)

# Which header declares each public name, by reflection: what tools/check_includes.py and
# docs/which-header.md read. The names are the same for both LVGL versions.
add_executable(lv_headers_reference ${PROJECT_SOURCE_DIR}/tools/reference/headers.cpp)
target_link_libraries(lv_headers_reference PRIVATE lvgl26::lvgl26 lvgl26_warnings)
# One constant evaluation over every name of lvgl26: 45 to 60 million steps, so a budget of its own
# at about twice that, as lv_reference has. Not for the analyzer: nothing in it runs.
set_source_files_properties(${PROJECT_SOURCE_DIR}/tools/reference/headers.cpp
                            PROPERTIES COMPILE_OPTIONS "-fno-analyzer;-fconstexpr-ops-limit=120000000")
add_test(NAME headers_data
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lv_headers_reference>
                 -DEXPECTED=${PROJECT_SOURCE_DIR}/docs/reference/headers.json
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_output.cmake)

# The settings as data: every key of a settings file, and its JSON Schema, by reflection.
add_executable(lv_settings_reference ${PROJECT_SOURCE_DIR}/tools/reference/settings.cpp)
target_link_libraries(lv_settings_reference PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME settings_reference
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lv_settings_reference>
                 -DEXPECTED=${PROJECT_SOURCE_DIR}/docs/reference/settings.json
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_output.cmake)
add_test(NAME settings_schema
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lv_settings_reference> -DARGUMENT=schema
                 -DEXPECTED=${PROJECT_SOURCE_DIR}/docs/reference/lvgl26-settings.schema.json
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_output.cmake)

# The pages in docs/ show only code that these tests have built, and no C names.
# The examples name nothing of LVGL's C API either.
find_package(Python3 COMPONENTS Interpreter)
# The files that the nightly compile-cost run measures and that no test is (tools/metrics/probes.json).
# They are compiled only by the run, and are in the compile database for tools/compile_cost.py.
add_library(cost_probes OBJECT EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/tools/probes/include_lv.cpp
  ${PROJECT_SOURCE_DIR}/tools/probes/one_label.cpp
  ${PROJECT_SOURCE_DIR}/tools/probes/ten_kinds.cpp
  ${PROJECT_SOURCE_DIR}/tools/probes/look_percent.cpp
  ${PROJECT_SOURCE_DIR}/examples/quick_start/main.cpp)
target_link_libraries(cost_probes PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
# The run-time probe of docs/research/visibility.md, run under Valgrind by the same job.
add_executable(visibility_probe EXCLUDE_FROM_ALL ${PROJECT_SOURCE_DIR}/tools/probes/visibility.cpp)
target_link_libraries(visibility_probe PRIVATE lvgl26::lvgl26 lvgl26_warnings)

if(Python3_FOUND)
  add_test(NAME docs_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/docs.py check)
  # The outline of a guide, its messages tied to compile-fail tests, and its rules.
  add_test(NAME docs_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_docs.py)
  # The README follows the code: no old form, no C name, its links, and the facts it repeats.
  add_test(NAME readme_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_readme.py)
  add_test(NAME readme_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_readme.py)
  # The hook that reminds an agent of the README before a commit of syntax or features.
  add_test(NAME readme_reminder_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_readme_reminder.py)
  # The rules of good code are data; the page "Writing good lvgl26 code" is written from it.
  add_test(NAME rules_page COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/rules.py
                                   --check-page ${PROJECT_SOURCE_DIR}/docs/rules.md)
  add_test(NAME rules_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_rules.py)
  # An example names nothing of LVGL's C API.
  add_test(NAME examples_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_examples.py)
  # Every test and fuzz target includes the header of each name of lvgl26 it uses
  # (docs/which-header.md); tools/check_examples.py holds the examples to their exact headers.
  file(GLOB_RECURSE lvgl26_checked_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/fuzz/*.cpp)
  # These leave out a header on purpose: the compiler must name it (lv/detail/missing.hpp).
  list(FILTER lvgl26_checked_sources EXCLUDE REGEX "/fail/missing_header_|/editor/missing_header.cpp$")
  add_test(NAME includes_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_includes.py
                                       ${lvgl26_checked_sources})
  add_test(NAME includes_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_includes.py)
  add_test(NAME order_properties_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_order_properties.py)
  # The format of UI code (tools/format_ui.py, .clang-format). Its lever tests need clang-format, which
  # only the editor image has; they skip where it is missing, and the job editor runs them.
  add_test(NAME editor_format_ui COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_format_ui.py)
  add_test(NAME call_form_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_call_form.py)
  # The examples are in the format of the repository (tools/format_ui.py): where clang-format is found, which is
  # the editor image, a file that differs fails the job.
  find_program(LVGL26_CLANG_FORMAT NAMES clang-format-21 clang-format-22 clang-format-23 clang-format)
  if(LVGL26_CLANG_FORMAT)
    add_test(NAME editor_format_examples
             COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/format_ui.py --check ${PROJECT_SOURCE_DIR}/examples)
    set_tests_properties(editor_format_examples PROPERTIES ENVIRONMENT CLANG_FORMAT=${LVGL26_CLANG_FORMAT})
  endif()
  # The build checks the includes of every file of a target that links lvgl26 (cmake/check_includes.cmake):
  # the check of tests/smoke.cpp left its stamp.
  if(LVGL26_CHECK_INCLUDES)
    # A program of its own that misses an include fails to build with the line, passes with
    # it, and checks nothing twice (tests/include_check/run.cmake).
    add_test(NAME include_check_build
             COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/include_check
                     -DLVGL=${lvgl_SOURCE_DIR} -DMP_UNITS=${mp-units_SOURCE_DIR} -DGENERATOR=${CMAKE_GENERATOR}
                     -DC_COMPILER=${CMAKE_C_COMPILER} -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/include_check/run.cmake)
    add_test(NAME includes_build_check
             COMMAND Python3::Interpreter -c "import pathlib, sys; sys.exit(0 if list(pathlib.Path(sys.argv[1]).glob('smoke.cpp.*.stamp')) else 1)"
                     ${CMAKE_BINARY_DIR}/lvgl26_includes/smoke)
  endif()
  add_test(NAME which_header_page COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_includes.py
                                          --check-page ${PROJECT_SOURCE_DIR}/docs/which-header.md)
  # The token files of the Signal theme are written from the design system.
  add_test(NAME signal_tokens_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/signal_tokens.py --check)
  # The copy of Signal's design file, its record (version and hash) and the written files agree.
  add_test(NAME signal_sync_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/signal_sync.py)
  add_test(NAME signal_sync_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_signal_sync.py)
  # The migration table of the status page is written from docs/migration/coverage.json.
  add_test(NAME migration_matrix COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/migration_matrix.py --check)
  add_test(NAME migration_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_migration_matrix.py)
  # Every component group of the looks example's token file is named by an lv::look in its source:
  # a group that nothing uses (a typo, a look that went unused) fails the test and is listed.
  add_test(NAME look_groups_example
           COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/look_groups.py
                   ${PROJECT_SOURCE_DIR}/examples/looks/tokens.json ${PROJECT_SOURCE_DIR}/examples/looks/screen.hpp
                   --reference ${PROJECT_SOURCE_DIR}/docs/reference/${LVGL26_REFERENCE_FILE})
  add_test(NAME look_groups_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_look_groups.py)
  add_test(NAME stack_budget_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_stack_budget.py)
  # No header does compile-time work where it is read (CLAUDE.md, the two rules on namespace scope).
  add_test(NAME eager_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_eager.py)
  add_test(NAME eager_check_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_eager.py)
  # The word counts that docs/looks.md states are those of the code it shows.
  add_test(NAME looks_words COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/code_words.py
                                    --check ${PROJECT_SOURCE_DIR}/docs/looks.md)
  # A regular expression of a test is one text: CTest splits it at an unescaped ';' and accepts any part.
  # The messages that the compile-fail tests pin can be read back (the documentation quotes them), and the tool is tested.
  add_test(NAME compile_fail_messages COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/compile_fail_messages.py ${CMAKE_BINARY_DIR} ${CMAKE_CTEST_COMMAND})
  add_test(NAME compile_fail_messages_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_compile_fail_messages.py)
  # How a text becomes the regular expression of a compile-fail test.
  add_test(NAME compile_fail_regex COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail_regex.cmake)
  add_test(NAME ctest_regex_lists COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_regex_lists.py ${CMAKE_BINARY_DIR} ${CMAKE_CTEST_COMMAND})
  add_test(NAME ctest_regex_lists_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_regex_lists.py)
  # The compile cost report: its parsing and rewriting, and one real file when the build is configured.
  add_test(NAME compile_report_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_compile_report.py)
  # The nightly compile-cost measure: its parsing, the judging of a night and the trend page.
  add_test(NAME compile_cost_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_compile_cost.py)
  # The nightly mutation run: which headers it mutates, its time budget and its summary.
  add_test(NAME mutation_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_mutation.py)
  # The longest symbol of that object file stays under a budget that no picture's pixels could fit in.
  set(LVGL26_SYMBOL_BUDGET 4096)
  add_test(NAME image_symbols COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_symbols.py
                                      --budget ${LVGL26_SYMBOL_BUDGET} $<TARGET_OBJECTS:image_symbols>)
  add_test(NAME symbols_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_symbols.py)
  add_test(NAME icons_names_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_icons_names.py)
  # The rewrite of token files from the forms before the Design Tokens Format 2025.10.
  add_test(NAME tokens_fix_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_dtcg_fix.py)
  # Every token file against the official Design Tokens Format schemas (needs the jsonschema package, which the CI image has).
  execute_process(COMMAND ${Python3_EXECUTABLE} -c "import jsonschema" RESULT_VARIABLE lvgl26_jsonschema_missing OUTPUT_QUIET ERROR_QUIET)
  if(lvgl26_jsonschema_missing)
    message(STATUS "tests: tokens_check, tokens_tools and settings_check are skipped: the Python package jsonschema is missing")
  else()
    add_test(NAME tokens_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_tokens.py)
    add_test(NAME tokens_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_tokens.py)
    # The settings files against their schema, and the keys the setup skill names.
    add_test(NAME settings_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_settings.py)
  endif()
endif()

# The fuzz target for runtime documents is also an ordinary program. Building it
# here keeps it compiling, and replaying its corpus under every preset (including
# the sanitizers) turns each input a fuzzer ever found into a regression test.
add_executable(fuzz_runtime_document ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/runtime_document.cpp)
target_link_libraries(fuzz_runtime_document PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(GLOB LVGL26_DOCUMENT_CORPUS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/corpus/document/*)
foreach(input ${LVGL26_DOCUMENT_CORPUS})
  get_filename_component(name ${input} NAME)
  add_test(NAME fuzz_corpus_document_${name} COMMAND fuzz_runtime_document ${input})
endforeach()

# The JSON reader's libFuzzer target, built here with a plain main so that its
# corpus (including every input that once crashed it) is replayed under every preset.
add_executable(fuzz_json_replay ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/json_reader.cpp
                                ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/replay_main.cpp)
target_link_libraries(fuzz_json_replay PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(GLOB LVGL26_JSON_CORPUS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/corpus/json/*)
add_test(NAME fuzz_corpus_json COMMAND fuzz_json_replay ${LVGL26_JSON_CORPUS})

# The MCP message handler takes whatever a client sends. Its fuzz target replays
# its corpus here in the same way.
add_executable(fuzz_mcp_message ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/mcp_message.cpp)
target_link_libraries(fuzz_mcp_message PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(GLOB LVGL26_MCP_CORPUS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/corpus/mcp/*)
foreach(input ${LVGL26_MCP_CORPUS})
  get_filename_component(name ${input} NAME)
  add_test(NAME fuzz_corpus_mcp_${name} COMMAND fuzz_mcp_message ${input})
endforeach()

# The stream serve() reads may be any bytes, a line without end among them. Its fuzz
# target replays its corpus here in the same way.
add_executable(fuzz_mcp_stream ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/mcp_stream.cpp)
target_link_libraries(fuzz_mcp_stream PRIVATE lvgl26::lvgl26 lvgl26_warnings)
file(GLOB LVGL26_STREAM_CORPUS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../fuzz/corpus/stream/*)
foreach(input ${LVGL26_STREAM_CORPUS})
  get_filename_component(name ${input} NAME)
  add_test(NAME fuzz_corpus_stream_${name} COMMAND fuzz_mcp_stream ${input})
endforeach()

# ---- editor support (tools/editor, cmake/editor.cmake) -----------------------------
#
# The stubs that let clangd read lvgl26 code must say what the real headers say.
# editor/api_uses.cpp uses every public name once. GCC compiles it here with the real
# headers; Clang reads it below with the stubs.
add_library(editor_api_uses OBJECT editor/api_uses.cpp)
target_compile_options(editor_api_uses PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/editor)
target_link_libraries(editor_api_uses PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)

# The public names, listed by reflection.
add_executable(lv_names ${PROJECT_SOURCE_DIR}/tools/editor/names.cpp)
target_link_libraries(lv_names PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
# Neither file is for the analyzer: nothing in them runs. See lv_reference above.
set_source_files_properties(editor/api_uses.cpp ${PROJECT_SOURCE_DIR}/tools/editor/names.cpp
                            PROPERTIES COMPILE_OPTIONS -fno-analyzer)

# Files that only Clang reads, with the stubs. They are never built: a target gives
# each one a line in compile_commands.json, which tools/editor/check.py takes its flags from.
add_library(editor_clang_only OBJECT EXCLUDE_FROM_ALL
            editor/wrong_enumerator.cpp editor/unknown_member.cpp editor/wrong_widget.cpp editor/wrong_look.cpp
            editor/missing_header.cpp
            editor/tokens_by_bytes.cpp ${CMAKE_CURRENT_BINARY_DIR}/editor/names_declared.cpp)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/editor/names_declared.cpp PROPERTIES GENERATED TRUE)
target_compile_options(editor_clang_only PRIVATE --embed-dir=${CMAKE_CURRENT_BINARY_DIR}/editor)
target_link_libraries(editor_clang_only PRIVATE lvgl26::lvgl26 lvgl26_test_data)
set_target_properties(editor_api_uses editor_clang_only PROPERTIES EXPORT_COMPILE_COMMANDS ON)
configure_file(data/tokens.json ${CMAKE_CURRENT_BINARY_DIR}/editor/unseen_tokens.json COPYONLY)

# A Clang that can read the stubs: version 21 or newer, and not Apple's, whose numbers
# mean something else. Clang 19 reads the stubs, but not mp-units 2.5.0, which the
# quick start includes. Clang 20 was not tried.
function(lvgl26_find_clang result names)
  find_program(${result} NAMES ${names} HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  if(${result})
    execute_process(COMMAND ${${result}} --version OUTPUT_VARIABLE said ERROR_QUIET)
    if(said MATCHES "Apple" OR NOT said MATCHES "version ([0-9]+)" OR CMAKE_MATCH_1 LESS 21)
      set(${result} ${result}-NOTFOUND CACHE FILEPATH "" FORCE)
    endif()
  endif()
endfunction()

if(Python3_FOUND)
  set(LVGL26_EDITOR ${PROJECT_SOURCE_DIR}/tools/editor)
  set(LVGL26_EDITOR_STUBS ${CMAKE_CURRENT_BINARY_DIR}/editor/stubs)
  add_test(NAME editor_tools COMMAND Python3::Interpreter ${LVGL26_EDITOR}/test_editor.py)
  # Every public name is declared in the stubs and used in editor/api_uses.cpp. It also
  # writes the file that names them all, for editor_names_declared.
  add_test(NAME editor_names
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:lv_names> -DPYTHON=${Python3_EXECUTABLE}
                   -DINVENTORY=${LVGL26_EDITOR}/inventory.py
                   -DUSING=${CMAKE_CURRENT_BINARY_DIR}/editor/names_declared.cpp
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/editor/names_test.cmake)
  set_tests_properties(editor_names PROPERTIES FIXTURES_SETUP editor_names)

  lvgl26_find_clang(LVGL26_CLANGXX "clang++;clang++-23;clang++-22;clang++-21")
  lvgl26_find_clang(LVGL26_CLANGD "clangd;clangd-23;clangd-22;clangd-21")
  if(NOT LVGL26_CLANGXX)
    message(STATUS "lvgl26: no clang++ of version 21 or newer: the editor_* tests that read the stubs are left out")
  else()
    add_test(NAME editor_stubs
             COMMAND Python3::Interpreter ${LVGL26_EDITOR}/generate.py
                     --reference ${PROJECT_SOURCE_DIR}/docs/reference/${LVGL26_REFERENCE_FILE}
                     --out ${LVGL26_EDITOR_STUBS} --real-include ${PROJECT_SOURCE_DIR}/include
                     --scan ${PROJECT_SOURCE_DIR}/examples --scan ${CMAKE_CURRENT_SOURCE_DIR}/editor
                     --tokens ${CMAKE_CURRENT_SOURCE_DIR}/data/tokens.json)
    set_tests_properties(editor_stubs PROPERTIES FIXTURES_SETUP editor_stubs)
    set(LVGL26_EDITOR_CHECK Python3::Interpreter ${LVGL26_EDITOR}/check.py --clang ${LVGL26_CLANGXX}
                            --commands ${CMAKE_BINARY_DIR}/compile_commands.json --stubs ${LVGL26_EDITOR_STUBS})
    # Every example, and every file a page of the documentation shows, reads without an
    # error. The quick start and the slice are projects of their own: they borrow flags.
    add_test(NAME editor_examples
             COMMAND ${LVGL26_EDITOR_CHECK} --like ${PROJECT_SOURCE_DIR}/examples/lvgl/get_started/slider.cpp
                     --examples ${PROJECT_SOURCE_DIR}/examples --docs ${PROJECT_SOURCE_DIR}/docs)
    add_test(NAME editor_api_uses COMMAND ${LVGL26_EDITOR_CHECK} ${CMAKE_CURRENT_SOURCE_DIR}/editor/api_uses.cpp)
    add_test(NAME editor_names_declared
             COMMAND ${LVGL26_EDITOR_CHECK} ${CMAKE_CURRENT_BINARY_DIR}/editor/names_declared.cpp)
    add_test(NAME editor_tokens_by_bytes
             COMMAND ${LVGL26_EDITOR_CHECK} ${CMAKE_CURRENT_SOURCE_DIR}/editor/tokens_by_bytes.cpp)
    # Mistakes are marked on the line they are written on.
    foreach(case wrong_enumerator unknown_member wrong_widget wrong_look missing_header)
      add_test(NAME editor_${case}
               COMMAND ${LVGL26_EDITOR_CHECK} --verify ${CMAKE_CURRENT_SOURCE_DIR}/editor/${case}.cpp)
    endforeach()
    set_tests_properties(editor_examples editor_api_uses editor_names_declared editor_tokens_by_bytes
                         editor_wrong_enumerator editor_unknown_member editor_wrong_widget editor_wrong_look
                         editor_missing_header
                         PROPERTIES FIXTURES_REQUIRED editor_stubs)
    set_property(TEST editor_names_declared APPEND PROPERTY FIXTURES_REQUIRED editor_names)
  endif()

  # A project turns editor support on with one line, and clangd then reads it. The
  # project is configured, not built. LVGL26_CLANGD may be empty: then clangd is not run.
  if(NOT LVGL26_SANITIZE AND NOT LVGL26_ANALYZE AND NOT LVGL26_COVERAGE)
    string(JOIN "\;" LVGL26_CONSUMER_OPTIONS
           -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
           -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DLVGL26_LVGL_REF=${LVGL26_LVGL_REF}
           -DFETCHCONTENT_SOURCE_DIR_LVGL26=${PROJECT_SOURCE_DIR}
           -DFETCHCONTENT_SOURCE_DIR_LVGL=${lvgl_SOURCE_DIR}
           -DFETCHCONTENT_SOURCE_DIR_MP-UNITS=${mp-units_SOURCE_DIR})
    set(LVGL26_CONSUMER_CLANGD)
    if(LVGL26_CLANGD)
      set(LVGL26_CONSUMER_CLANGD -DCLANGD=${LVGL26_CLANGD})
    endif()
    add_test(NAME editor_support
             COMMAND ${CMAKE_COMMAND} -DSOURCE=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/editor/consumer
                     -DGENERATOR=${CMAKE_GENERATOR} -DOPTIONS=${LVGL26_CONSUMER_OPTIONS} ${LVGL26_CONSUMER_CLANGD}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/editor/consumer_test.cmake)
    set_tests_properties(editor_support PROPERTIES TIMEOUT 600)
    # clangd's background index names the header of a name that a file misses. Indexing lvgl26
    # takes minutes, so the test runs every night (ctest -L nightly) and the pull request run
    # leaves it out (-LE nightly).
    if(LVGL26_CLANGD)
      add_test(NAME editor_quick_fix
               COMMAND ${CMAKE_COMMAND} -DSOURCE=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/editor/quick_fix
                       -DGENERATOR=${CMAKE_GENERATOR} -DOPTIONS=${LVGL26_CONSUMER_OPTIONS} -DCLANGD=${LVGL26_CLANGD}
                       -DPYTHON=${Python3_EXECUTABLE} -P ${CMAKE_CURRENT_SOURCE_DIR}/editor/quick_fix_test.cmake)
      set_tests_properties(editor_quick_fix PROPERTIES TIMEOUT 1800 LABELS nightly)
    endif()
  endif()
endif()

# Lists of options: see tests/options.cpp.
lvgl26_compile_fail_message(options_empty_item constexpr "lv: option 2 of the list is empty. LVGL takes the options as one text separated by line breaks, and an empty one would show as a blank row.")
lvgl26_compile_fail_message(options_line_break constexpr "lv: option 2 of the list, \"75%\\n50%\", holds a line break. The list is the separator: write each option as an item, .options = {\"100%\", \"75%\", \"50%\"}, not \"100%\\n75%\\n50%\".")
lvgl26_compile_fail_message(options_but_other_enum static_assert "lv: lv::but in lv::options_of names enumerators of the enum given to lv::options_of: lv::options_of<quality, lv::but<quality::draft>>.")
lvgl26_compile_fail_message(options_but_all constexpr "lv: lv::options_of<quality> has no option left. Every enumerator is left out by lv::but, or the enum has none.")
lvgl26_compile_fail_message(options_not_an_enum static_assert "lv: lv::options_of<E> takes an enum, one option for each of its enumerators.")
lvgl26_compile_fail_message(options_other_enum constexpr "lv: .selected follows a state of sheet, and the options come from quality. A state follows the choice of the options of its own enum: use .options = lv::options_of<sheet>.")
lvgl26_compile_fail_message(options_plain_list constexpr "lv: .selected follows a state of sheet, and the options of this widget do not come from that enum. Give the widget .options = lv::options_of<sheet>, or bind a state that holds the index.")
lvgl26_compile_fail_message(options_enum_state_elsewhere static_assert "lv: a state of an enum follows only the choice of a dropdown or a roller: .selected = lv::bind(state), beside .options = lv::options_of<the same enum>.")
lvgl26_compile_fail_message(options_roller_without_mode static_assert "lv: wrong value for this property. It takes a list of options ({\"100%\", \"75%\", \"50%\"}, or lv::options_of<an enum>), or a text with a line break between the options, then the name of an LVGL value in quotes, such as \"center\". You gave a text, a text, a text. The property takes 2 values in braces, and this has 3.")
lvgl26_compile_fail_message(options_roller_two_texts constexpr "lv: roller_mode has no value 'Tue'. Its values: normal, infinite.")
lvgl26_compile_fail_message(options_row_enum static_assert "lv: a field of a row that holds an enum cannot follow a whole number. The choice of options follows a state of the enum: .selected = lv::bind(state).")

# lv::look: every mistake in a look is a compile error that says what is valid.
lvgl26_compile_fail_message(look_unknown_group constexpr "lv: the look 'queue.crad' has no group 'crad' in 'queue'. The groups there: card, details. Did you mean 'card'?")
lvgl26_compile_fail_message(look_unknown_root constexpr "lv: the look 'tile-pritn' has no group 'tile-pritn' in the token file. The groups there: <<names>>. Did you mean 'tile_print'?")
lvgl26_compile_fail_message(look_entry_no_property constexpr "lv: the look entry 'colour' of 'typo' fits no style property. Nearest style properties: <<names>>. A shadow, border, transition or typography token may have any name; anything else is named for its property.")
lvgl26_compile_fail_message(look_wrong_type constexpr "lv: the look entry 'radius' of 'wrongtype' is a color, but the style property 'radius' takes a dimension (such as 8px). Change the entry's \$type or its value in the token file.")
lvgl26_compile_fail_message(look_bad_enumerator constexpr "lv: text_align has no value 'centre'. Its values: auto, left, center, right.")
lvgl26_compile_fail_message(look_size_one_value constexpr "lv: the look entry 'size' of 'sizeone' sets several properties from several values, and a token holds one. Write each of them: width and height.")
lvgl26_compile_fail_message(look_font_for_color constexpr "lv: the look entry 'text_color' of 'font_for_color' is a typography, but the style property 'text_color' takes a color. Change the entry's \$type or its value in the token file.")
lvgl26_compile_fail_message(look_group_not_layer constexpr "lv: in the look 'layered' the group 'cheked' is neither a part nor a state. Did you mean 'checked'? A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-cheked' beside 'a', used as lv::look<\"a a-cheked\">.")
lvgl26_compile_fail_message(look_group_in_layer constexpr "lv: in the look 'layeredin' the group 'extra' is neither a part nor a state. A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-extra' beside 'a', used as lv::look<\"a a-extra\">.")
lvgl26_compile_fail_message(look_names_a_token constexpr "lv: the look 'tile.radius' names a token, not a group: 'tile.radius' holds one value. A look is a group of tokens, written lv::look<\"tile\">, and the style property is its entry 'radius'.")
lvgl26_compile_fail_message(look_empty constexpr "lv: lv::look<\"  \"> names no look. Write the path of a group of the token file: lv::look<\"queue.card\">. The groups there: <<names>>.")
lvgl26_compile_fail_message(look_layer_far_typo constexpr "lv: in the look 'layerfar' the group 'chekd' is neither a part nor a state. Did you mean 'checked'? A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-chekd' beside 'a', used as lv::look<\"a a-chekd\">.")
lvgl26_compile_fail_message(look_unknown_group_close constexpr "lv: the look 'queue.cardd' has no group 'cardd' in 'queue'. The groups there: card, details. Did you mean 'card'?")
lvgl26_compile_fail_message(look_layer_part_typo constexpr "lv: in the look 'layerpart' the group 'scrolbar' is neither a part nor a state. Did you mean 'scrollbar'? A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-scrolbar' beside 'a', used as lv::look<\"a a-scrolbar\">.")
lvgl26_compile_fail_message(look_layer_any_typo constexpr "lv: in the look 'layerany' the group 'anyy' is neither a part nor a state. A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-anyy' beside 'a', used as lv::look<\"a a-anyy\">.")
lvgl26_compile_fail_message(look_layer_state_typo constexpr "lv: in the look 'layeralt' the group 'alte' is neither a part nor a state. Did you mean 'alt'? A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-alte' beside 'a', used as lv::look<\"a a-alte\">.")
lvgl26_compile_fail_message(look_layer_keyword_typo constexpr "lv: in the look 'layerkeyword' the group 'defaultx' is neither a part nor a state. Did you mean 'default'? A group nested in a look is a layer: a part (main, scrollbar, indicator, knob, selected, items, cursor, custom_first) or a state (default, alt, checked, focused, focus_key, edited, hovered, pressed, scrolled, disabled, user_1, user_2, user_3, user_4). A variant of a look is a sibling group, 'a-defaultx' beside 'a', used as lv::look<\"a a-defaultx\">.")
lvgl26_compile_fail_message(look_several_values_not_size constexpr "lv: the look entry 'enabled' of 'enabledone' sets several properties from several values, and a token holds one. Write each of them: see lv::style.")
lvgl26_compile_fail_message(look_ext_lone_percent constexpr "lv: the \$extensions entry 'width' of 'lonepercent' is the text '%', and the style property 'width' takes a dimension (such as 8px). \$extensions holds LVGL values by name and sizes in percent: write this one as a token, with its \$type.")
lvgl26_compile_fail_message(font_of_unknown_root constexpr "lv: the font 'txt.label' has no name 'txt' in the token file. The names there: <<names>>. Did you mean 'text'?")
lvgl26_compile_fail_message(font_of_through_a_token constexpr "lv: the font 'text.label.big' goes through 'text.label', which is a token and holds no group.")
lvgl26_compile_fail_message(look_empty_name constexpr "lv: the look 'tile..print' has an empty name between its dots. Write lv::look<\"queue.card\">.")
# lv::font_of and lv::draw: a font named in a file of its own is checked against the token file.
lvgl26_compile_fail_message(font_of_unknown_name constexpr "lv: the font 'text.lable' has no name 'lable' in 'text'. The names there: label, small. Did you mean 'label'?")
lvgl26_compile_fail_message(font_of_not_typography constexpr "lv: the font 'color.blue' names a color, not a typography token. A font comes from a typography token: lv::font_of<\"text.title\">.")
lvgl26_compile_fail_message(font_of_no_design static_assert "lv: no design is registered for lv::font_of. Name the token file once, in a header that every file includes: constexpr auto tok = lv::tokens<tokens_json>; template <> inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};")
lvgl26_compile_fail_message(font_draw_unknown_name constexpr "lv: the font 'text.title' has no name 'title' in 'text'. The names there: label, small.")
lvgl26_compile_fail_message(signal_fonts_and_type static_assert "lv: lv/signal_type.hpp and lv/signal_fonts.hpp both give Signal's fonts, one drawn in each file and one linked. Include one of them.")
lvgl26_compile_fail_message(signal_type_and_fonts static_assert "lv: lv/signal_fonts.hpp and lv/signal_type.hpp both give Signal's fonts, one linked and one drawn in each file. Include one of them.")
lvgl26_compile_fail_message(look_no_design static_assert "lv: no design is registered for lv::look. Name the token file once, before the first look, in a header that every file includes: constexpr auto tok = lv::tokens<tokens_json>; template <> inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok}; If you did name it, the naming comes after a look that has already read the design: move it above every lv::look, as lv::cfg must come first.")
lvgl26_compile_fail_message(look_design_too_late static_assert "lv: no design is registered for lv::look. Name the token file once, before the first look, in a header that every file includes: constexpr auto tok = lv::tokens<tokens_json>; template <> inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok}; If you did name it, the naming comes after a look that has already read the design: move it above every lv::look, as lv::cfg must come first.")
lvgl26_compile_fail_message(look_string_token constexpr "lv: the look entry 'flex_flow' of 'stringtoken' is a string token, and a string is no standard value for the style property 'flex_flow', which takes a string that names an LVGL value. An LVGL value by name (\"flex_flow\": \"row\") goes in \"\$extensions\": {\"io.github.filipsajdak.lvgl26\": {...}} of the group.")
lvgl26_compile_fail_message(look_percent_value constexpr "lv: the look entry 'width' of 'percentvalue' is a percentage, and Design Tokens Format 2025.10 has no percentage. Write a size in pixels as the \$value, for design tools, and the percentage in \"\$extensions\": {\"io.github.filipsajdak.lvgl26\": {\"width\": \"50%\"}} of the group.")
lvgl26_compile_fail_message(look_extends constexpr "lv: the group of the look 'extends' has \"\$extends\", which Design Tokens Format 2025.10 has and lvgl26 does not read yet. Write the entries of the group out, or use the sibling groups of a variant: lv::look<\"a a-b\">.")
lvgl26_compile_fail_message(look_ext_unknown_property constexpr "lv: the \$extensions entry 'colour' of 'extunknown' fits no style property. Nearest style properties: <<names>>.")
lvgl26_compile_fail_message(look_ext_percent_property constexpr "lv: the \$extensions entry 'pad_all' of 'extpad' is a percentage, and LVGL takes one for width, height, min_width, min_height, max_width, max_height, x, y, transform_width, transform_height, translate_x, translate_y only.")
lvgl26_compile_fail_message(look_ext_text constexpr "lv: the \$extensions entry 'radius' of 'exttext' is the text 'big', and the style property 'radius' takes a dimension (such as 8px). \$extensions holds LVGL values by name and sizes in percent: write this one as a token, with its \$type.")
lvgl26_compile_fail_message(look_ext_percent_digits constexpr "lv: the \$extensions entry 'width' of 'extpercent' is '5x%', and a percentage is written as whole digits and %, such as \"50%\".")
lvgl26_compile_fail_message(look_alpha_wrong_kind constexpr "lv: the look entry 'radius' of 'alpharadius' is a color with an alpha, but the style property 'radius' takes a dimension (such as 8px). Change the entry's \$type or its value in the token file.")
lvgl26_compile_fail_message(look_ext_not_string constexpr "lv: design tokens: the value of 'width' in \$extensions 'io.github.filipsajdak.lvgl26' is not a string. Write an LVGL value by its name (\"flex_flow\": \"row\") or a size in percent (\"width\": \"50%\")")

# A name of an opt-in header used with lv/lv.hpp alone names its header (lv/detail/missing.hpp).
lvgl26_compile_fail(missing_header_list "lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>")
lvgl26_compile_fail(missing_header_each "lv::each is in <lv/list.hpp>: add #include <lv/list.hpp>")
lvgl26_compile_fail(missing_header_for_each "lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>")
lvgl26_compile_fail(missing_header_for_each_items "lv::for_each is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>")
lvgl26_compile_fail(missing_header_for_each_field "lv::for_each_field is in <lv/repeat.hpp>: add #include <lv/repeat.hpp>")
lvgl26_compile_fail(missing_header_theme "lv::theme is in <lv/theme.hpp>: add #include <lv/theme.hpp>")
lvgl26_compile_fail(missing_header_look "lv::look is in <lv/look.hpp>: add #include <lv/look.hpp>")
lvgl26_compile_fail(missing_header_mode_look "lv::mode_look is in <lv/mode_look.hpp>: add #include <lv/mode_look.hpp>")
lvgl26_compile_fail_message(mode_look_shape_property constexpr "lv: the mode look 'tile' has another shape in the design 'dark_design' than in 'day_design': the property 'border_width' is set in the first design only. Both token files hold the group with the same entries, as the themes of lv::display::follow have one shape.")
lvgl26_compile_fail_message(mode_look_shape_layer constexpr "lv: the mode look 'tile' has another shape in the design 'dark_design' than in 'day_design': it has 0 layer(s) (a part or a state), the first design has 1. Both token files hold the group with the same entries, as the themes of lv::display::follow have one shape.")
lvgl26_compile_fail_message(mode_look_unknown_group constexpr "lv: the look 'page' has no group 'page' in the token file. The groups there: <<names>>. Did you mean 'panel'?")
lvgl26_compile_fail_message(mode_look_no_designs static_assert "lv: no designs are named for lv::mode_look. Name the token file of each mode once, in a header that every file includes: template <> inline constexpr lv::mode_designs_of<day, dark> lv::mode_designs<lv::override>{}; with lv::design<day> and lv::design<dark> specialized as lv::design<lv::override> is for lv::look.")
lvgl26_compile_fail(missing_header_image_file "lv::image_file is in <lv/image.hpp>: add #include <lv/image.hpp>")
lvgl26_compile_fail(missing_header_mask_file "lv::mask_file is in <lv/image.hpp>: add #include <lv/image.hpp>")
lvgl26_compile_fail(missing_header_encoded_image "lv::encoded_image is in <lv/encoded_image.hpp>: add #include <lv/encoded_image.hpp>")
lvgl26_compile_fail(missing_header_runtime "lv::runtime is in <lv/runtime.hpp>: add #include <lv/runtime.hpp>")
lvgl26_compile_fail(missing_header_one_of "lv::one_of is in <lv/one_of.hpp>: add #include <lv/one_of.hpp>")
lvgl26_compile_fail(missing_header_pointer "lv::pointer is in <lv/pointer.hpp>: add #include <lv/pointer.hpp>")

# lv::shown and lv::when (lv/shown.hpp).
lvgl26_compile_fail(shown_not_enum "lv::shown follows an lv::state or an lv::computed of an enum or a bool")
lvgl26_compile_fail(shown_missing_value "lv::shown does not cover: ink. Add an arm for each. lv::is<value>\\(\\) shows nothing for it.")
lvgl26_compile_fail(shown_duplicate_value "lv::shown has two arms for home. Each value needs one. one arm may name several")
lvgl26_compile_fail(shown_tree_twice "lv::shown shows the tree .*a in two arms. Name both values in one arm: lv::is<a, b>")
lvgl26_compile_fail(shown_other_enum "lv::shown follows a state of page, and an arm names a value of look")
lvgl26_compile_fail(shown_not_an_arm "lv::shown takes arms: lv::is<value>\\(tree\\)")
lvgl26_compile_fail(shown_otherwise_not_last "lv::shown: lv::otherwise must be the last arm")
lvgl26_compile_fail(shown_two_otherwise "lv::shown has more than one lv::otherwise arm")
lvgl26_compile_fail(shown_needless_otherwise "lv::shown already covers every value, so its lv::otherwise arm can never be shown")
lvgl26_compile_fail(shown_sibling "lv::shown is the only child of its widget. Give it a widget of its own")
lvgl26_compile_fail(shown_on_shown_other_tree "the function of .on_shown takes the lv::held of another tree than its arm shows. Write void f\\(lv::held<.*a>\\)")
lvgl26_compile_fail(shown_inline_tree "the tree of an arm of lv::shown is a constexpr variable of its own")
lvgl26_compile_fail(shown_in_each_row "a row of lv::each cannot hold a node that keeps run-time storage yet")
lvgl26_compile_fail(shown_alive_zero "lv::shown needs a slot for the page it shows: .alive = 1 or more.")
lvgl26_compile_fail(shown_alive_one_with_transition "a page that enters or leaves is alive beside the other one, so .enter and .leave need .alive = 2")
lvgl26_compile_fail(shown_alive_one_with_back_transition "a page that enters or leaves is alive beside the other one, so .enter and .leave need .alive = 2")
lvgl26_compile_fail(shown_alive_one_with_arm_transition "a page that enters or leaves is alive beside the other one, so .enter and .leave need .alive = 2")
lvgl26_compile_fail(shown_page_on_shown_in_arm "a page of another file takes its .on_shown in its own file: constinit const lv::page home_page = lv::page_of<ui, \\{.on_shown = &f\\}>")
lvgl26_compile_fail(shown_transition_two_properties "contract predicate is false in constant expression")
lvgl26_compile_fail(page_of_wrong_hook "lv::page_of<ui, \\{.on_shown = ..., .on_front = ...\\}>: the function of .on_shown takes the lv::held of another tree than the page.s. Write void f\\(lv::held<.*a>\\)")
lvgl26_compile_fail(shown_on_front_wrong_shape "lv::shown: .on_shown and .on_front take a function void f\\(lv::held<tree>\\), where tree is the tree of the arm")
lvgl26_compile_fail(shown_on_front_other_tree "the function of .on_front takes the lv::held of another tree than its arm shows. Write void f\\(lv::held<.*a>\\)")
lvgl26_compile_fail(page_of_wrong_front "the function of .on_front takes the lv::held of another tree than the page.s")
lvgl26_compile_fail(when_not_bool "lv::when follows an lv::state or an lv::computed of a bool")
# The children as arguments: a wrong one is named, with the form that works.
lvgl26_compile_fail_message(children_not_a_child static_assert "lv: after the properties come the children, each a widget or an lv::each, lv::shown, lv::when or lv::for_each: lv::group({...}, lv::label({...}), lv::button({...})).")
lvgl26_compile_fail_message(children_alone_not_a_child static_assert "lv: after the properties come the children, each a widget or an lv::each, lv::shown, lv::when or lv::for_each: lv::group({...}, lv::label({...}), lv::button({...})).")
lvgl26_compile_fail_message(each_row_not_a_tree static_assert "lv: the last argument of lv::each is its row template, one widget: lv::each(tracks, &track::id, {...}, track_row).")
lvgl26_compile_fail_message(each_row_then_more static_assert "lv: the last argument of lv::each is its row template, one widget: lv::each(tracks, &track::id, {...}, track_row).")
lvgl26_compile_fail_message(each_plain_row_not_a_tree static_assert "lv: lv::each takes the options in braces, then the row template: lv::each(tracks, &track::id, {...}, track_row).")
lvgl26_compile_fail_message(when_second_not_a_tree deleted "lv::when follows an lv::state or an lv::computed of a bool, and shows one tree: lv::when(online, online_ui).")
lvgl26_compile_fail(shown_arms_not_enum "lv::shown follows an lv::state or an lv::computed of an enum or a bool")
lvgl26_compile_fail(shown_under_place "lv::place leaves the widgets to their parent, and this tree keeps run-time storage")
lvgl26_compile_fail(missing_header_shown "lv::shown is in <lv/shown.hpp>: add #include <lv/shown.hpp>")
lvgl26_compile_fail(missing_header_when "lv::when is in <lv/shown.hpp>: add #include <lv/shown.hpp>")
lvgl26_compile_fail(missing_header_shown_arm "lv::is<...>\\(tree\\) is an arm of lv::shown, in <lv/shown.hpp>: add #include <lv/shown.hpp>")

# Icons (lv/icons.hpp): a name that is no icon lists the nearest.
lvgl26_compile_fail_message(icon_unknown_name constexpr "lv: there is no icon 'arow-left' in Phosphor. Did you mean 'arrow-left'? The nearest names: arrow-left, caret-left, align-left, copyleft, arrow-down.")
lvgl26_compile_fail_message(icon_far_name constexpr "lv: there is no icon 'steam-engine' in Phosphor. The nearest names: steam-logo, gear-fine, bell-ringing, champagne, dots-nine.")

# Gestures (lv/gesture.hpp).
lvgl26_compile_fail_message(hold_progress_not_percent deleted "lv: the progress of lv::hold is an lv::state<lv::percent>: lv::state<lv::percent> progress{lv::percent{0}}. It goes from 0 to 100 while the finger stays.")
lvgl26_compile_fail(swiped_not_a_view "no matching function for call to 'swiped\\(int\\)'")
