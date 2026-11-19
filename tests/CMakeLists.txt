# Where #embed finds the test data.
add_library(lvgl26_test_data INTERFACE)
target_compile_options(lvgl26_test_data INTERFACE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/data)
# The Barlow files of the site are the test fonts: `#embed "fonts/Barlow-Medium.ttf"`.
target_compile_options(lvgl26_test_data INTERFACE --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
# The compile-fail tests that use quantities need mp-units too.
target_link_libraries(lvgl26_test_data INTERFACE mp-units::mp-units)

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

lvgl26_test(smoke)
# The library in a program without exceptions and run-time type information.
lvgl26_test(no_exceptions)
target_compile_options(no_exceptions PRIVATE -fno-exceptions -fno-rtti)
lvgl26_test(l1_views)
lvgl26_test(json)
lvgl26_test(runtime_ui)
lvgl26_test(match)
lvgl26_test(for_each)
lvgl26_test(mounted_storage)
target_link_libraries(mounted_storage PRIVATE lvgl26_test_data)  # look_tokens.json
# unmount() of a tree that is not mounted, and mount() of one that is, end the program.
foreach(case unmount_not_mounted mount_twice unmount_after_parent)
  add_test(NAME mounted_storage_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:mounted_storage> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
lvgl26_test(look)
lvgl26_test(start_style)
lvgl26_test(style_add)
target_link_libraries(look PRIVATE lvgl26_test_data)
# Two files of one program name looks, with the registration in a header they share.
add_executable(look_link look_link_main.cpp look_link_cards.cpp)
target_link_libraries(look_link PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME look_link COMMAND look_link)
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
lvgl26_test(options)
# LVGL says which setters take options separated by line breaks in its doc comments; the names
# that lvgl26 goes by must agree with them.
lvgl26_test(options_docs)
# A state of an enum given to set() at run time breaks its contract.
add_test(NAME options_violation_enum_state_in_set
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:options> -DCASE=set
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
lvgl26_test(animate)
lvgl26_test(enum_names)
lvgl26_test(unsigned_values)
add_test(NAME unsigned_values_violation_negative
         COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:unsigned_values> -DCASE=negative
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
lvgl26_test(application)
lvgl26_test(theme)
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

# A tree of 112 widgets, and the guard of its compile time. The limit is the number of
# steps the compiler may take in one constant evaluation; GCC's default is 33554432.
# This file needs 5.8 million, on both LVGL versions; the limit below is 6.5 million. With nodes that held a whole
# props struct, as they first did, it needed more than 170 million, and a tree of 100
# widgets took the compiler 8 GB of memory. The limit leaves room for LVGL to grow,
# not for that to come back.
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
                    value_range_orientation
                    ASSETS img_bar_bg img_bar_indicator)
lvgl26_widget_ports(switch event orientation state)
lvgl26_widget_ports(checkbox bind_checked event states text)
lvgl26_widget_ports(led brightness color styling)
lvgl26_widget_ports(line points)
lvgl26_widget_ports(obj bind_style)
lvgl26_widget_ports(buttonmatrix event map)
lvgl26_widget_ports(dropdown bind_value direction event text)
# LVGL draws a roller with a left shift of a negative number (lv_roller.c, draw_main), which a
# sanitizer stops on. The picture is compared in the builds that have none.
if(NOT LVGL26_SANITIZE)
  lvgl26_widget_ports(roller bind_value options)
endif()
lvgl26_widget_ports(spinbox bind_value format rollover styling value)
lvgl26_widget_ports(textarea placeholder)
lvgl26_widget_ports(image inner_align src transformations ASSETS img_example_lvgl_logo)
lvgl26_widget_ports(scale modes range styling ticks)
lvgl26_widget_ports(keyboard modes popovers textarea)

add_executable(font font.cpp)
target_compile_options(font PRIVATE --embed-dir=${lvgl_SOURCE_DIR})  # two of LVGL's own font files
target_link_libraries(font PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font COMMAND font A:${CMAKE_CURRENT_SOURCE_DIR}/ref/font.png)
# The fonts are the bytes that one evaluation of the whole font made.
add_executable(font_bytes font_bytes.cpp)
target_compile_options(font_bytes PRIVATE --embed-dir=${lvgl_SOURCE_DIR})
target_link_libraries(font_bytes PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font_bytes COMMAND font_bytes)

add_executable(image image.cpp)
target_link_libraries(image PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME image COMMAND image A:${CMAKE_CURRENT_SOURCE_DIR}/ref/image.png)
lvgl26_test(image_alone)
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

lvgl26_compile_fail(dump_not_a_sink "dump takes a sink: something to call with a std::string_view")
lvgl26_compile_fail(dump_loaded_not_a_sink "dump takes a sink: something to call with a std::string_view")
lvgl26_compile_fail(unknown_property "widget 'label' has no property 'txt' to set. Its own: .*text")
lvgl26_compile_fail(unknown_widget "there is no widget 'lable'. Valid: .*label")
lvgl26_compile_fail(big_capture "an event handler may capture nothing, or one pointer-sized")
# A part that constrains its parameters with the public concepts (docs/concepts.md).
# The whole message of a handler that holds state, sentence by sentence: each says what to do.
string(CONCAT handler_state_message "It takes a function or a lambda that captures nothing: `&function`, `\\[\\] \\{ \\.\\.\\. \\}` or "
    "`\\[\\]\\(lv::event e\\) \\{ \\.\\.\\. \\}`\\. A handler in a constexpr tree is a plain function pointer, "
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
lvgl26_compile_fail(list_without_rows "a list holds at least one row")
lvgl26_compile_fail(each_two_rows "lv::each takes one row template")
lvgl26_compile_fail(each_row_storage "a row of lv::each cannot hold a node that keeps run-time storage")
lvgl26_compile_fail(mounted_detach_storage "keeps run-time storage inside its lv::mounted, so its widgets cannot")
lvgl26_compile_fail(place_storage "lv::place leaves the widgets to their parent, and this tree keeps run-time")
lvgl26_compile_fail(storage_kind_shape "a node kind with a member `storage` declares it as")
lvgl26_compile_fail(each_without_row "lv::each needs a row template")
lvgl26_compile_fail(each_plain_key "a key names one row")
lvgl26_compile_fail(each_not_a_list "lv::each takes an lv::list and the member of its rows that holds the key")
lvgl26_compile_fail(each_key_of_another_row "lv::each takes an lv::list and the member of its rows that holds the key")
lvgl26_compile_fail(each_twice_in_one_widget "a widget holds at most one lv::each")
lvgl26_compile_fail(row_field_outside_each "follows a field of a row, and this node is not in a row")
lvgl26_compile_fail(row_field_plain_int "a field of a plain integer does not say what the number is")
lvgl26_compile_fail(row_field_string_view "a std::string_view field has no ending zero")
lvgl26_compile_fail(row_field_not_a_value "a field of a row follows like a state")
lvgl26_compile_fail(row_field_function "follows a data member of a row")
lvgl26_compile_fail(row_field_wrong_property "this property cannot be bound to a state of that type")
lvgl26_compile_fail(row_field_two_values "takes more than one value and cannot follow a field of a row")
lvgl26_compile_fail(row_view_unknown_id "the row template has no id 'titel'. Its ids: title")
lvgl26_compile_fail(row_template_duplicate_id "the id 'a' is used twice in one row template")
lvgl26_compile_fail(part_repeat_capturing_lambda "a_part_captures_nothing__name_what_it_needs_as_a_template_argument")
lvgl26_compile_fail(for_each_unknown_but "lv::but names a value that is no enumerator of the enum")
lvgl26_compile_fail(for_each_part_rejects "does not accept this element as a template argument")
lvgl26_compile_fail(for_each_id_in_part "a part of lv::for_each sets the id 'tile'")
lvgl26_compile_fail(for_each_nothing_left "has no enumerator left to repeat")
lvgl26_compile_fail(for_each_not_an_enum "lv::for_each<E> takes an enum")
lvgl26_compile_fail(for_each_part_not_a_widget "the part of lv::for_each returns a widget")
lvgl26_compile_fail(for_each_field_without_caption "finds no member with a caption")
lvgl26_compile_fail(for_each_empty_array "the array is empty")
lvgl26_compile_fail(caption_not_a_literal "a caption is a string literal")
lvgl26_compile_fail(like_missing_member "finds no member named 'scan'")
lvgl26_compile_fail(each_selected_wrong_type ".selected follows the key of the selected row. Bind a state of the key's type")
lvgl26_compile_fail(each_selected_and_checked ".selected gives the selected row the checked state, and this row template sets .checked itself")
lvgl26_compile_fail(props_without_braces "made_with_braces")
lvgl26_compile_fail(props_unknown_name "no match for call to .*factory<lv::detail::fixed<4>.\"arc\"")
lvgl26_compile_fail(props_needs_unit "It takes a width or x .*You gave int")
lvgl26_compile_fail(props_wrong_unit "It takes an angle in whole degrees \\(90 \\* lv::deg\\). You gave lv::length")
lvgl26_compile_fail(props_bool_as_int "It takes a whole number \\(not bool, not floating point\\). You gave bool")
lvgl26_compile_fail(props_narrowing "It takes a whole number \\(not bool, not floating point\\). You gave double")
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
lvgl26_compile_fail(style_needs_token "It takes a design token \\(for example tok.space.md\\) or lv::raw\\(...\\), holding a length .*You gave lv::length")
lvgl26_compile_fail(style_raw_color "It takes a design token .*You gave lv_color_t")
lvgl26_compile_fail(theme_needs_token "It takes a design token .*You gave lv::length")
lvgl26_compile_fail(theme_not_a_value "a theme or an lv::on cannot hold the style property 'enabled': LVGL keeps no value for it in a style")
lvgl26_compile_fail(encoded_image_not_a_picture "lv::encoded_image takes a PNG or a JPEG file, and this is neither")
lvgl26_compile_fail(encoded_image_progressive "this JPEG file is progressive, or of another form than baseline")
lvgl26_compile_fail(encoded_image_empty "this picture is 0 x 0 pixels. LVGL holds a picture of 1 to 65535 pixels each way")
lvgl26_compile_fail(encoded_image_cut "this JPEG file is damaged: it does not say how large its picture is")
lvgl26_compile_fail(encoded_image_needs_png "LVGL is built without its PNG decoder. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_PNG ON\\)")
target_compile_options(fail_encoded_image_needs_png PRIVATE -ULV_USE_LODEPNG)  # as a build without the option
lvgl26_compile_fail(encoded_image_needs_jpeg "LVGL is built without its JPEG decoder. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_JPEG ON\\)")
target_compile_options(fail_encoded_image_needs_jpeg PRIVATE -ULV_USE_TJPGD)
lvgl26_compile_fail(image_not_a_png "this is not a PNG file. Embed one")
lvgl26_compile_fail(image_too_large "this picture is 200 x 100 pixels, and the compiler decodes at most 16384 .*raise lv::config::compile_time_image_pixels")
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
lvgl26_compile_fail(theme_bound_font "a theme and an lv::on hold constants, and a font bound with lv::bind is drawn on the device")
lvgl26_compile_fail(on_unknown_part "widgets have no part 'knop'. The parts: main, scrollbar, indicator, knob, selected, items, cursor, custom_first\\.")
lvgl26_compile_fail(on_state_as_part "'pressed' is a state. lv::on takes the part first, then the state")
lvgl26_compile_fail(on_unknown_state "widgets have no state 'presed'. The states: default, alt, checked, focused, .*pressed")
lvgl26_compile_fail(on_part_as_state "'indicator' is a part. lv::on takes the part first, then the state")
lvgl26_compile_fail(on_unknown_name "'knop' is neither a part nor a state. The parts: main, .* The states: default, ")
lvgl26_compile_fail(on_any "'any' is neither a part nor a state")
lvgl26_compile_fail(on_inside_on "an lv::on cannot hold another lv::on. Put them side by side")
lvgl26_compile_fail(on_not_a_value "a theme or an lv::on cannot hold the style property 'enabled'")
lvgl26_compile_fail(on_bound_font "a theme and an lv::on hold constants, and a font bound with lv::bind is drawn on the device")
lvgl26_compile_fail(on_in_part_slot "lv::on goes in .style, because it names its own part and state")
lvgl26_compile_fail(theme_on_in_part_slot "lv::on goes in .style, because it names its own part and state")
lvgl26_compile_fail(font_needs_token "It takes a font from a typography token .*or lv::raw\\(font\\) for a font that no token names")
lvgl26_compile_fail(font_not_a_font "this is not a TrueType font file. Embed a .ttf file")
lvgl26_compile_fail(font_no_outlines "this font has no TrueType outlines")
lvgl26_compile_fail(font_lacks_table "this font file lacks a table that lvgl26 reads")
lvgl26_compile_fail(font_size_out_of_range "a font size is from 4 px to 255 px")
lvgl26_compile_fail(font_no_glyph "the font file has no glyph for the character U\\+0416")
lvgl26_compile_fail(font_glyph_too_large "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_glyph_too_tall "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_too_many_characters "a font holds at most 255 characters")
lvgl26_compile_fail(font_bitmap_too_large "the pixels of this font take 1538941 bytes. LVGL's font format holds at most 1048575")
lvgl26_compile_fail(typeface_no_file "the typeface has no file for the family 'Montserrat' at weight 700. Add it")
lvgl26_compile_fail(typeface_too_many_files "a typeface has at most eight files")
lvgl26_compile_fail(signal_numerals_not_a_numeral "the typeface has no file for the family 'Barlow' at weight 500. Add it")
lvgl26_compile_fail(style_wrong_token "It takes a design token .*holding a length .*You gave lv::token<lv_color_t>")
lvgl26_compile_fail(token_bad_alias "design tokens: the alias {color.brand} points at nothing")
lvgl26_compile_fail(token_bad_color "design tokens: a color has a character that is not a hex digit")
lvgl26_compile_fail(token_color_alpha "design tokens: the color '#01020380' has an alpha channel, which a color token does not carry. Write it as #rrggbb and give the opacity its own token")
lvgl26_compile_fail(token_color_no_hash "design tokens: the color 'x123456' is not #rgb or #rrggbb")
lvgl26_compile_fail(token_color_too_long "design tokens: the color '#1122334455' is not #rgb or #rrggbb")
lvgl26_compile_fail(token_color_too_short "design tokens: the color '#' is not #rgb or #rrggbb")
lvgl26_compile_fail(token_color_five_digits "design tokens: the color '#12345' is not #rgb or #rrggbb")
lvgl26_compile_fail(token_color_two_components "design tokens: a color needs a hex string or three components")
lvgl26_compile_fail(token_duration_unit_h "design tokens: the duration unit 'h' is not ms or s")
lvgl26_compile_fail(token_duration_unit_us "design tokens: the duration unit 'us' is not ms or s")
lvgl26_compile_fail(token_length_unit_bang "design tokens: the dimension unit '!' is not px, rem or %")
lvgl26_compile_fail(token_unit_without_number "design tokens: 'px' has no number before its unit")
lvgl26_compile_fail(token_bad_weight "design tokens: the font weight 'thin' is not a number, normal or bold")
lvgl26_compile_fail(token_gradient_not_a_list "design tokens: a gradient token must be a list of at most eight stops")
lvgl26_compile_fail(token_alias_too_deep "design tokens: the alias {g.t9} goes in a circle")
lvgl26_compile_fail(token_alias_circle "design tokens: the alias {g.[ab]} goes in a circle")
lvgl26_compile_fail(token_value_at_end "contract predicate is false")
lvgl26_compile_fail(token_bezier_three_numbers "design tokens: a cubicBezier token must be a list of four numbers")
lvgl26_compile_fail(token_bezier_not_a_list "design tokens: a cubicBezier token must be a list of four numbers")
lvgl26_compile_fail(token_gradient_nine_stops "design tokens: a gradient token must be a list of at most eight stops")
lvgl26_compile_fail(token_value_past_end "contract predicate is false")
lvgl26_compile_fail(token_bad_unit "design tokens: the dimension unit 'pt' is not px, rem or %")
lvgl26_compile_fail(token_misspelled "has no member named 'primry'")
lvgl26_compile_fail(token_bad_shadow "design tokens: a composite token has no 'blur'")
lvgl26_compile_fail(units_decidegrees_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_number_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_number_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_too_large "contract predicate is false in constant expression")
lvgl26_compile_fail(units_decidegrees_too_large_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_decidegrees_not_tenths "contract predicate is false in constant expression")
lvgl26_compile_fail(values_token_decidegrees_not_tenths_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(units_fraction_for_whole_angle "this takes an angle in whole degrees, and the value may have a fraction of a degree")
lvgl26_compile_fail(props_fraction_for_whole_angle "It takes an angle in whole degrees \\(90 \\* lv::deg\\). You gave lv::decidegrees")
lvgl26_compile_fail(bind_angle_to_decidegrees "this property is in tenths of a degree, and a state of lv::angle holds whole degrees")
lvgl26_compile_fail(angle_header_not_read "'lv_dial_set_rotation' takes or returns an angle, and only LVGL's doc comment says .*angle_header_not_read.cpp. Add that header to angle_headers")
lvgl26_compile_fail(angle_mark_not_found "no doc comment in LVGL's headers says '0.1 degree' any more")
lvgl26_compile_fail(bind_wrong_type "this property cannot be bound to a state of that type")
lvgl26_compile_fail(bind_format_not_text "is for text properties only")
lvgl26_compile_fail(state_bad_type "a state holds a quantity")
lvgl26_compile_fail(bind_two_values "this property cannot follow a state")
lvgl26_compile_fail(config_too_late "redefinition of .*lv::cfg<lv::override>")
lvgl26_compile_fail(state_plain_int "give it a unit with mp-units: .lv::state speed.0 \\* km / h.;.")
lvgl26_compile_fail(units_wrong_axis "It takes a width or x .*not a vertical length")
lvgl26_compile_fail(units_bool_count "no match for 'operator\\*'")
lvgl26_compile_fail(bind_printf_format "a text binding format needs exactly one {} for the value")
lvgl26_compile_fail(quantity_wrong_dimension "no match for .operator=")
lvgl26_compile_fail(quantity_lossy "no match for .operator=")
lvgl26_compile_fail(quantity_floating "a quantity in a state needs an integer number for now")
lvgl26_compile_fail(computed_plain_int "the function of an lv::computed returns a plain integer, which does not say what the number is. Return a quantity")
lvgl26_compile_fail(computed_bad_result "the function of an lv::computed returns double, which a state cannot hold. Return a quantity")
lvgl26_compile_fail(computed_wrong_arguments "the function of an lv::computed takes the values of its dependencies, in their order: \\(lv::counter, bool\\). Take each by value")
lvgl26_compile_fail(computed_not_a_state "a dependency of an lv::computed must be an lv::state or an lv::computed that outlives it. Pass the object itself")
lvgl26_compile_fail(computed_temporary "a dependency of an lv::computed must be an lv::state or an lv::computed that outlives it. Pass the object itself")
lvgl26_compile_fail(computed_constexpr "an lv::computed cannot be a constexpr variable: binding it writes to it. Write .constinit lv::computed., as for a state")
lvgl26_compile_fail(effect_wrong_arguments "the function of an lv::effect takes the values of its dependencies, in their order: \\(lv::counter\\). Take each by value")
lvgl26_compile_fail(effect_not_a_state "a dependency of an lv::effect must be an lv::state or an lv::computed that outlives it. Pass the object itself")
lvgl26_compile_fail(bind_computed_wrong_type "this property cannot be bound to a state of that type")
lvgl26_compile_fail(first_mixed_handlers "in lv::first, give a handler to every item")
lvgl26_compile_fail(animate_not_a_number "invalid initialization of reference of type .lv::detail::lazy_animatable<lv::detail::fixed<6>.\"label\"")
lvgl26_compile_fail(animate_needs_unit "It takes a width or x .*You gave int")
lvgl26_compile_fail(animate_one_end "an animation needs both ends: .property = {from, to}")
lvgl26_compile_fail(animate_mixed_units "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_mixed_units_folded "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_size_content "contract predicate is false in constant expression")
lvgl26_compile_fail(animate_then_bad_handler "no matching function for call to .*then")
lvgl26_compile_fail(post_not_callable "no matching function for call to .lv::ui_scheduler::post")
lvgl26_compile_fail(match_missing_case "lv::match does not cover: sport. Add an arm for each")
lvgl26_compile_fail(match_duplicate_case "lv::match has two arms for eco")
lvgl26_compile_fail(match_redundant_otherwise "already covers every value, so its lv::otherwise arm can never match")
lvgl26_compile_fail(match_needs_otherwise "cannot list every value, so it needs a last arm")
lvgl26_compile_fail(match_two_otherwise "lv::match has more than one lv::otherwise arm")
lvgl26_compile_fail(match_otherwise_not_last "lv::otherwise must be the last arm")
lvgl26_compile_fail(match_not_an_arm "lv::match takes arms: lv::is<...>.then.handler. or lv::otherwise.then.handler.")
lvgl26_compile_fail(match_bad_handler "A handler takes the matched value or nothing")
lvgl26_compile_fail(match_stored "must be one expression: the matcher refers to the value")
lvgl26_compile_fail(match_not_comparable "no matching function for call to .match.blob")
lvgl26_compile_fail(set_wrong_argument "no matching function for call to .*set<.value.>")
lvgl26_compile_fail(then_bad_handler "no matching function for call to .*then")
lvgl26_compile_fail(weak_not_a_view "requires +view_value<V> class lv::weak")
lvgl26_compile_fail(owned_not_a_view "requires +view_value<V> class lv::owned")
lvgl26_compile_fail(enumerator_unknown_name "align has no value 'centre'. Its values: default, top_left")
lvgl26_compile_fail(enumerator_unknown_name_beside_text "roller_mode has no value .endless.. Its values: normal, infinite")
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

# The example of docs/repetition.md: it runs, and tests/for_each.cpp checks what it repeats.
add_executable(example_repeat ${PROJECT_SOURCE_DIR}/examples/repeat/main.cpp)
target_link_libraries(example_repeat PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_repeat COMMAND example_repeat)

# The example of docs/mounting.md: a tree in a variable, in a global, in memory set aside.
add_executable(example_mounting ${PROJECT_SOURCE_DIR}/examples/mounting/main.cpp)
target_link_libraries(example_mounting PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_mounting COMMAND example_mounting)

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

# The second example of docs/fonts.md: a font drawn on the device.
add_executable(example_scaled_font ${PROJECT_SOURCE_DIR}/examples/scaled_font/main.cpp)
target_compile_options(example_scaled_font PRIVATE --embed-dir=${PROJECT_SOURCE_DIR}/examples/fonts
                                                   --embed-dir=${PROJECT_SOURCE_DIR}/site/media)
target_link_libraries(example_scaled_font PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME example_scaled_font COMMAND example_scaled_font)

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

# The pages in docs/ show only code that these tests have built, and no C names.
# The examples name nothing of LVGL's C API either.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME docs_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/docs.py check)
  # An example names nothing of LVGL's C API.
  add_test(NAME examples_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_examples.py)
  # The token files of the Signal theme are written from the design system.
  add_test(NAME signal_tokens_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/signal_tokens.py --check)
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
  # The word counts that docs/looks.md states are those of the code it shows.
  add_test(NAME looks_words COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/code_words.py
                                    --check ${PROJECT_SOURCE_DIR}/docs/looks.md)
  # The compile cost report: its parsing and rewriting, and one real file when the build is configured.
  add_test(NAME compile_report_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_compile_report.py)
  # The nightly mutation run: which headers it mutates, its time budget and its summary.
  add_test(NAME mutation_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_mutation.py)
  # Every token file against the official Design Tokens Format schemas (needs the jsonschema package, which the CI image has).
  execute_process(COMMAND ${Python3_EXECUTABLE} -c "import jsonschema" RESULT_VARIABLE lvgl26_jsonschema_missing OUTPUT_QUIET ERROR_QUIET)
  if(lvgl26_jsonschema_missing)
    message(STATUS "tests: tokens_check and tokens_tools are skipped: the Python package jsonschema is missing")
  else()
    add_test(NAME tokens_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_tokens.py)
    add_test(NAME tokens_tools COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/test_check_tokens.py)
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
    foreach(case wrong_enumerator unknown_member wrong_widget wrong_look)
      add_test(NAME editor_${case}
               COMMAND ${LVGL26_EDITOR_CHECK} --verify ${CMAKE_CURRENT_SOURCE_DIR}/editor/${case}.cpp)
    endforeach()
    set_tests_properties(editor_examples editor_api_uses editor_names_declared editor_tokens_by_bytes
                         editor_wrong_enumerator editor_unknown_member editor_wrong_widget editor_wrong_look
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
  endif()
endif()

# Lists of options: see tests/options.cpp.
lvgl26_compile_fail(options_empty_item "option 2 of the list is empty")
lvgl26_compile_fail(options_line_break "option 2 of the list, .*holds a line break. The list is the separator")
lvgl26_compile_fail(options_but_other_enum "lv::but in lv::options_of names enumerators of the enum given to lv::options_of")
lvgl26_compile_fail(options_but_all "lv::options_of<quality> has no option left")
lvgl26_compile_fail(options_not_an_enum "lv::options_of<E> takes an enum")
lvgl26_compile_fail(options_other_enum ".selected follows a state of sheet, and the options come from quality")
lvgl26_compile_fail(options_plain_list ".selected follows a state of sheet, and the options of this widget do not come from that enum")
lvgl26_compile_fail(options_enum_state_elsewhere "a state of an enum follows only the choice of a dropdown or a roller")
lvgl26_compile_fail(options_roller_without_mode "It takes a list of options .* then lv_roller_mode_t")
lvgl26_compile_fail(options_roller_two_texts "roller_mode has no value 'Tue'")
lvgl26_compile_fail(options_row_enum "a field of a row that holds an enum cannot follow a whole number")

# lv::look: every mistake in a look is a compile error that says what is valid.
lvgl26_compile_fail(look_unknown_group "the look 'queue.crad' has no group 'crad' in 'queue'. The groups there: card, details. Did you mean 'card'\\?")
lvgl26_compile_fail(look_unknown_root "the look 'tile-pritn' has no group 'tile-pritn' in the token file. The groups there: .*Did you mean 'tile_print'\\?")
lvgl26_compile_fail(look_entry_no_property "the look entry 'colour' of 'typo' fits no style property. Nearest style properties: ")
lvgl26_compile_fail(look_wrong_type "the look entry 'radius' of 'wrongtype' is a color, but the style property 'radius' takes a dimension")
lvgl26_compile_fail(look_bad_enumerator "text_align has no value 'centre'. Its values: ")
lvgl26_compile_fail(look_size_one_value "'size' of 'sizeone' sets several properties from several values, and a token holds one. Write each of them: width and height")
lvgl26_compile_fail(look_font_for_color "the look entry 'text_color' of 'font_for_color' is a typography, but the style property 'text_color' takes a color")
lvgl26_compile_fail(look_group_not_layer "in the look 'layered' the group 'cheked' is neither a part nor a state. Did you mean 'checked'\\? A group nested in a look is a layer: a part \\(main, .*\\) or a state \\(default, .*\\). A variant of a look is a sibling group, 'a-cheked' beside 'a'")
lvgl26_compile_fail(look_group_in_layer "in the look 'layeredin' the group 'extra' is neither a part nor a state")
lvgl26_compile_fail(look_names_a_token "the look 'tile.radius' names a token, not a group")
lvgl26_compile_fail(look_empty "lv::look<\"  \"> names no look. Write the path of a group of the token file")
lvgl26_compile_fail(look_empty_name "has an empty name between its dots")
lvgl26_compile_fail(look_no_design "no design is registered for lv::look. Name the token file once")
lvgl26_compile_fail(look_design_too_late "conflicting declaration .*lv::design<lv::override>")
lvgl26_compile_fail(look_string_token "the look entry 'flex_flow' of 'stringtoken' is a string token, and a string is no standard value for the style property 'flex_flow'.*goes in \"\\$extensions\": {\"io.github.filipsajdak.lvgl26\"")
lvgl26_compile_fail(look_percent_value "the look entry 'width' of 'percentvalue' is a percentage, and Design Tokens Format 2025.10 has no percentage. Write a size in pixels as the .value")
lvgl26_compile_fail(look_extends "the group of the look 'extends' has \"\\$extends\", which Design Tokens Format 2025.10 has and lvgl26 does not read yet")
lvgl26_compile_fail(look_ext_unknown_property "the .extensions entry 'colour' of 'extunknown' fits no style property. Nearest style properties: ")
lvgl26_compile_fail(look_ext_percent_property "the .extensions entry 'pad_all' of 'extpad' is a percentage, and LVGL takes one for width, height, min_width")
lvgl26_compile_fail(look_ext_text "the .extensions entry 'radius' of 'exttext' is the text 'big', and the style property 'radius' takes a dimension")
lvgl26_compile_fail(look_ext_percent_digits "is '5x%', and a percentage is written as whole digits and %")
lvgl26_compile_fail(look_alpha_wrong_kind "the look entry 'radius' of 'alpharadius' is a color with an alpha, but the style property 'radius' takes a dimension")
lvgl26_compile_fail(look_ext_not_string "the value of 'width' in .extensions 'io.github.filipsajdak.lvgl26' is not a string")
