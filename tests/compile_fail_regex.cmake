# The escaping of lvgl26_message_regex, checked by cmake -P: each case is the text, and the regular
# expression it becomes.
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/compile_fail.cmake)

function(expect form text expected)
  lvgl26_message_regex(got "${form}" "${text}")
  if(NOT got STREQUAL expected)
    message(FATAL_ERROR "text '${text}': expected\n  [${expected}]\ngot\n  [${got}]")
  endif()
endfunction()

expect(constexpr "plain words." "constexpr message: plain words\\.\n")
expect(static_assert "a (b) [c] {d} e+f?g*h|i^j$k" "static assertion failed: a \\(b\\) \\[c\\] \\{d\\} e\\+f\\?g\\*h\\|i\\^j\\$k\n")
expect(constexpr "a; b" "constexpr message: a\; b\n")
expect(constexpr "back\\slash" "constexpr message: back\\\\slash\n")
expect(deleted "x" "use of deleted function '[^\n]*': x\n")
expect(constexpr "Its own: <<names>>." "constexpr message: Its own: [a-z_0-9]+(, [a-z_0-9]+)*\\.\n")
expect(constexpr "<<number>> px" "constexpr message: -?[0-9]+(\\.[0-9]+)? px\n")
message(STATUS "lvgl26_message_regex: ok")
