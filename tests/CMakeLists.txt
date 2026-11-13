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
function(lvgl26_compile_fail name message)
  add_library(fail_${name} OBJECT EXCLUDE_FROM_ALL fail/${name}.cpp)
  target_link_libraries(fail_${name} PRIVATE lvgl26::lvgl26 lvgl26_test_data)
  add_test(NAME fail_${name}
           COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target fail_${name})
  set_tests_properties(fail_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${message}")
endfunction()

lvgl26_test(smoke)
lvgl26_test(l1_views)
lvgl26_test(json)
lvgl26_test(runtime_ui)
lvgl26_test(match)
lvgl26_test(state)
target_link_libraries(state PRIVATE mp-units::mp-units)
lvgl26_test(state_plain)
lvgl26_test(task)

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

add_executable(styles styles.cpp)
target_link_libraries(styles PRIVATE lvgl26::lvgl26 lvgl26_warnings)
add_test(NAME styles COMMAND styles A:${CMAKE_CURRENT_SOURCE_DIR}/ref/styled.png)

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
lvgl26_compile_fail(props_out_of_range "contract predicate is false in constant expression")
lvgl26_compile_fail(style_needs_token "It takes a design token \\(for example tok.space.md\\) or lv::raw\\(...\\), holding a length .*You gave lv::length")
lvgl26_compile_fail(style_raw_color "It takes a design token .*You gave lv_color_t")
lvgl26_compile_fail(style_wrong_token "It takes a design token .*holding a length .*You gave lv::token<lv_color_t>")
lvgl26_compile_fail(token_bad_alias "design tokens: the alias {color.brand} points at nothing")
lvgl26_compile_fail(token_bad_color "design tokens: a color has a character that is not a hex digit")
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
lvgl26_compile_fail(first_mixed_handlers "in lv::first, give a handler to every item")
lvgl26_compile_fail(match_missing_case "lv::match does not cover: sport. Add an arm for each")
lvgl26_compile_fail(match_duplicate_case "lv::match has two arms for eco")
lvgl26_compile_fail(match_redundant_otherwise "already covers every value, so its lv::otherwise arm can never match")
lvgl26_compile_fail(match_needs_otherwise "cannot list every value, so it needs a last arm")
lvgl26_compile_fail(match_otherwise_not_last "lv::otherwise must be the last arm")
lvgl26_compile_fail(match_not_an_arm "lv::match takes arms: lv::is<...>.then.handler. or lv::otherwise.then.handler.")
lvgl26_compile_fail(match_bad_handler "A handler takes the matched value or nothing")
lvgl26_compile_fail(match_stored "must be one expression: the matcher refers to the value")
lvgl26_compile_fail(match_not_comparable "no matching function for call to .match.blob")
lvgl26_compile_fail(set_wrong_argument "no matching function for call to .*set<.value.>")
lvgl26_compile_fail(then_bad_handler "no matching function for call to .*then")
lvgl26_compile_fail(weak_not_a_view "constraint")
lvgl26_compile_fail(mount_not_a_tree "no matching function for call to .mount<not_a_tree>")
