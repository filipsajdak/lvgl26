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
target_link_libraries(computed PRIVATE mp-units::mp-units)
lvgl26_test(state_plain)
lvgl26_test(angle_units)
lvgl26_test(task)
lvgl26_test(animate)
lvgl26_test(enum_names)
lvgl26_test(application)
lvgl26_test(theme)
lvgl26_test(scaled_font)
target_link_libraries(scaled_font PRIVATE lvgl26_test_data)
# A font size is more than nothing.
foreach(case zero negative)
  add_test(NAME scaled_font_violation_${case}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:scaled_font> -DCASE=${case}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_violation.cmake)
endforeach()
lvgl26_test(signal)
target_link_libraries(theme PRIVATE lvgl26_test_data)
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
# This file needs 5.9 million, on both LVGL versions. With nodes that held a whole
# props struct, as they first did, it needed more than 170 million, and a tree of 100
# widgets took the compiler 8 GB of memory. The limit leaves room for LVGL to grow,
# not for that to come back.
add_executable(large_tree large_tree.cpp)
target_link_libraries(large_tree PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
target_compile_options(large_tree PRIVATE -fconstexpr-ops-limit=9000000)
# Not for the analyzer: it follows every path through the 112 functions that build this
# tree, which takes it minutes. They are the functions the small trees are built with,
# and those it does check. A source file's options come last, after -fanalyzer.
set_source_files_properties(large_tree.cpp PROPERTIES COMPILE_OPTIONS -fno-analyzer)
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

add_executable(font font.cpp)
target_compile_options(font PRIVATE --embed-dir=${lvgl_SOURCE_DIR})  # two of LVGL's own font files
target_link_libraries(font PRIVATE lvgl26::lvgl26 lvgl26_warnings lvgl26_test_data)
add_test(NAME font COMMAND font A:${CMAKE_CURRENT_SOURCE_DIR}/ref/font.png)

add_executable(styles styles.cpp)
target_link_libraries(styles PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME styles COMMAND styles A:${CMAKE_CURRENT_SOURCE_DIR}/ref/styled.png)

lvgl26_compile_fail(dump_not_a_sink "dump takes a sink: something to call with a std::string_view")
lvgl26_compile_fail(dump_loaded_not_a_sink "dump takes a sink: something to call with a std::string_view")
lvgl26_compile_fail(unknown_property "widget 'label' has no property 'txt' to set. Its own: .*text")
lvgl26_compile_fail(unknown_widget "there is no widget 'lable'. Valid: .*label")
lvgl26_compile_fail(big_capture "an event handler may capture nothing, or one pointer-sized")
lvgl26_compile_fail(unknown_id "the tree has no id 'titel'. Its ids: title")
lvgl26_compile_fail(duplicate_id "the id 'a' is used twice in one tree")
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
lvgl26_compile_fail(theme_not_a_value "a theme cannot hold the style property 'enabled': LVGL keeps no value for it in a style")
lvgl26_compile_fail(scaled_font_needs_ttf "lv::scaled_font needs LVGL's TinyTTF, which is off. Turn it on in CMake, before lvgl26 is added: set\\(LVGL26_TTF ON\\)")
target_compile_options(fail_scaled_font_needs_ttf PRIVATE -ULV_USE_TINY_TTF)  # as a build without the option
lvgl26_compile_fail(scaled_font_raw_needs_ttf "lv::scaled_font needs LVGL's TinyTTF, which is off")
target_compile_options(fail_scaled_font_raw_needs_ttf PRIVATE -ULV_USE_TINY_TTF)
lvgl26_compile_fail(scaled_font_plain_size "the size of a font comes from a typography token")
lvgl26_compile_fail(scaled_font_bound_to_number "lv::bind\\(font\\) is for a font property of a style")
lvgl26_compile_fail(theme_bound_font "a theme holds constants, and a font bound with lv::bind is drawn on the device")
lvgl26_compile_fail(font_needs_token "It takes a font from a typography token .*or lv::raw\\(font\\) for a font that no token names")
lvgl26_compile_fail(font_not_a_font "this is not a TrueType font file. Embed a .ttf file")
lvgl26_compile_fail(font_no_outlines "this font has no TrueType outlines")
lvgl26_compile_fail(font_lacks_table "this font file lacks a table that lvgl26 reads")
lvgl26_compile_fail(font_size_out_of_range "a font size is from 4 px to 255 px")
lvgl26_compile_fail(font_no_glyph "the font file has no glyph for the character U\\+0416")
lvgl26_compile_fail(font_glyph_too_large "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_glyph_too_tall "a glyph of this font is larger than LVGL's font format holds")
lvgl26_compile_fail(font_too_many_characters "a font holds at most 255 characters")
lvgl26_compile_fail(typeface_no_file "the typeface has no file for the family 'Montserrat' at weight 700. Add it")
lvgl26_compile_fail(typeface_too_many_files "a typeface has at most eight files")
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
lvgl26_compile_fail(weak_not_a_view "constraint")
lvgl26_compile_fail(enumerator_unknown_name "align has no value 'centre'. Its values: default, top_left")
lvgl26_compile_fail(mount_not_a_tree "no matching function for call to .mount<not_a_tree>")
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

# The pages in docs/ show only code that these tests have built, and no C names.
# The examples name nothing of LVGL's C API either.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME docs_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/docs.py check)
  # An example names nothing of LVGL's C API.
  add_test(NAME examples_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check_examples.py)
  # The token files of the Signal theme are written from the design system.
  add_test(NAME signal_tokens_check COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/signal_tokens.py --check)
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
