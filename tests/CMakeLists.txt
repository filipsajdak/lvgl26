# Where #embed finds the test data.
add_library(lvgl26_test_data INTERFACE)
target_compile_options(lvgl26_test_data INTERFACE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR}/data)
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
lvgl26_test(l1_views)
lvgl26_test(json)
lvgl26_test(runtime_ui)
lvgl26_test(match)
lvgl26_test(state)
lvgl26_test(units)
lvgl26_test(values)
target_link_libraries(state PRIVATE mp-units::mp-units)
lvgl26_test(computed)
target_link_libraries(computed PRIVATE mp-units::mp-units)
lvgl26_test(state_plain)
lvgl26_test(task)
lvgl26_test(animate)
# lv::ui_scheduler is the one part that other threads call.
find_package(Threads REQUIRED)
lvgl26_test(ui_scheduler)
target_link_libraries(ui_scheduler PRIVATE Threads::Threads)

# The agent kit writes its pictures into the build directory.
add_executable(agent_kit agent_kit.cpp)
target_link_libraries(agent_kit PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME agent_kit COMMAND agent_kit ${CMAKE_CURRENT_BINARY_DIR})

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
lvgl26_compile_fail(props_unknown_name "no match for call to .*factory<lv::detail::fixed<4>.\"arc\"")
lvgl26_compile_fail(props_needs_unit "It takes a width or x .*You gave int")
lvgl26_compile_fail(props_wrong_unit "It takes an angle \\(90 \\* lv::deg\\). You gave lv::length")
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
lvgl26_compile_fail(mount_not_a_tree "no matching function for call to .mount<not_a_tree>")

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
