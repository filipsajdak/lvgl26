# The test editor_names: the public names that reflection lists, checked against the
# editor stubs.
#
#   cmake -DPROGRAM=<lv_names> -DPYTHON=<python3> -DINVENTORY=<tools/editor/inventory.py>
#         -DUSING=<the C++ file to write> -P names_test.cmake
execute_process(COMMAND ${PROGRAM} COMMAND ${PYTHON} ${INVENTORY} --using ${USING}
                RESULTS_VARIABLE results OUTPUT_VARIABLE output ERROR_VARIABLE output)
message("${output}")
foreach(result IN LISTS results)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "editor_names: the stubs and the public names differ. Declare each name in "
                        "tools/editor/stubs and use it in tests/editor/api_uses.cpp.")
  endif()
endforeach()
