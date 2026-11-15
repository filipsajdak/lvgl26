# Runs PROGRAM with the argument CASE and expects it to end with a contract violation.
# A script, because CTest counts a program that aborts as failed whatever it printed.
execute_process(COMMAND ${PROGRAM} ${CASE}
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(result EQUAL 0 OR NOT output MATCHES "contract violation")
  message(FATAL_ERROR "expected a contract violation, got result '${result}':\n${output}")
endif()
message(STATUS "ended with a contract violation, as it must")
