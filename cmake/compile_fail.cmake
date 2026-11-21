# The text of a compile-fail test, written as the message a user reads, turned into the regular
# expression CTest checks. tests/CMakeLists.txt, lvgl26_compile_fail_message.
#
# The text is verbatim: the signs of a regular expression in it are escaped here, so a message reads
# in the file as it reads on the screen. A part that the compiler computes, such as a list of names,
# is a placeholder from the table below: a tight pattern, never `.*`. The regular expression ends at
# the end of the line, so a message with text after it fails the test.

# One row for each placeholder: <<name>> in a text becomes its pattern (CTest's regular expression
# dialect has no look-ahead and no \d).
set(LVGL26_PLACEHOLDERS names widgets number)
set(LVGL26_PLACEHOLDER_names "[a-z_0-9]+(, [a-z_0-9]+)*")  # a list of identifiers
set(LVGL26_PLACEHOLDER_widgets "obj, ([a-z_]+, )*label, ([a-z_]+, )*[a-z_]+")  # LVGL's widgets: obj first, label among them
set(LVGL26_PLACEHOLDER_number "-?[0-9]+(\\.[0-9]+)?")

# The text that each kind of message follows on its line of GCC's output: the severity too, so that a message
# that becomes a warning fails the test.
set(LVGL26_FORM_constexpr "error: constexpr message: ")
set(LVGL26_FORM_static_assert "error: static assertion failed: ")
set(LVGL26_FORM_deleted "error: use of deleted function '[^\n]*': ")

# lvgl26_message_regex(<result> <form> <text>)
function(lvgl26_message_regex result form text)
  if(NOT DEFINED LVGL26_FORM_${form})
    message(FATAL_ERROR "lvgl26_compile_fail_message: the form is constexpr, static_assert or deleted, not '${form}'")
  endif()
  string(REGEX REPLACE "([][+.*()^$?{}|\\\\])" "\\\\\\1" escaped "${text}")
  string(REPLACE ";" "\;" escaped "${escaped}")  # CTest reads an unescaped ; as the end of an alternative
  foreach(name ${LVGL26_PLACEHOLDERS})
    string(REPLACE "<<${name}>>" "${LVGL26_PLACEHOLDER_${name}}" escaped "${escaped}")
  endforeach()
  if(escaped MATCHES "<<[a-z]+>>")
    message(FATAL_ERROR "lvgl26_compile_fail_message: unknown placeholder in '${text}'")
  endif()
  set(${result} "${LVGL26_FORM_${form}}${escaped}\n" PARENT_SCOPE)
endfunction()
