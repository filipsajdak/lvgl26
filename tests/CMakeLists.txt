function(lvgl26_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE lvgl26::lvgl26)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# A file that must not compile, and the message the user must see.
function(lvgl26_compile_fail name message)
  add_library(fail_${name} OBJECT EXCLUDE_FROM_ALL fail/${name}.cpp)
  target_link_libraries(fail_${name} PRIVATE lvgl26::lvgl26)
  add_test(NAME fail_${name}
           COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target fail_${name})
  set_tests_properties(fail_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${message}")
endfunction()

lvgl26_test(smoke)
lvgl26_test(l1_views)

# LVGL v9.6 and the v10 pin render the same pixels, so one reference serves both.
add_executable(l2_tree l2_tree.cpp)
target_link_libraries(l2_tree PRIVATE lvgl26::lvgl26)
add_test(NAME l2_tree COMMAND l2_tree A:${CMAKE_CURRENT_SOURCE_DIR}/ref/slice.png)

lvgl26_compile_fail(unknown_property "widget 'label' has no property 'txt' to set. Its own: .*text")
lvgl26_compile_fail(unknown_widget "there is no widget 'lable'. Valid: .*label")
lvgl26_compile_fail(big_capture "an event handler may capture nothing, or one pointer-sized")
lvgl26_compile_fail(unknown_id "the tree has no id 'titel'. Its ids: title")
lvgl26_compile_fail(duplicate_id "the id 'a' is used twice in one tree")
lvgl26_compile_fail(tree_unknown_property "widget 'arc' has no property 'rnge' to set")
