# Runs PROGRAM and fails when it fails. Its output is dropped: the host build of
# examples/wasm writes a whole frame to standard output.
execute_process(COMMAND ${PROGRAM} RESULT_VARIABLE result OUTPUT_QUIET)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "${PROGRAM} ended with ${result}")
endif()
