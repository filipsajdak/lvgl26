# The test include_check_build: configures the program beside this file in a directory of its
# own, writes its header without and then with the include of lv::list, and builds only the
# check of its target each time. Arguments: ROOT (lvgl26), WORK (a new directory), LVGL and
# MP_UNITS (the sources that the build already fetched), GENERATOR.
set(app ${WORK}/source)
file(REMOVE_RECURSE ${WORK})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/ DESTINATION ${app} PATTERN run.cmake EXCLUDE)

function(write_header with_include)
  set(text "#pragma once\n\n")
  if(with_include)
    string(APPEND text "#include <lv/list.hpp>\n")
  endif()
  string(APPEND text "#include <lv/lv.hpp>\n\nstruct track {\n  int id;\n};\ninline lv::list<track, 4> rows;\n")
  file(WRITE ${app}/app/parts.hpp "${text}")
endfunction()

function(build expect_success expected_text)
  execute_process(COMMAND ${CMAKE_COMMAND} --build ${WORK}/build --target app_lvgl26_includes
                  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(expect_success AND NOT result EQUAL 0)
    message(FATAL_ERROR "the check failed where it must pass:\n${output}")
  endif()
  if(NOT expect_success AND result EQUAL 0)
    message(FATAL_ERROR "the check passed where a header is missing:\n${output}")
  endif()
  if(NOT output MATCHES "${expected_text}")
    message(FATAL_ERROR "expected '${expected_text}' in:\n${output}")
  endif()
endfunction()

write_header(OFF)
execute_process(COMMAND ${CMAKE_COMMAND} -S ${app} -B ${WORK}/build -G ${GENERATOR}
                        -DLVGL26_ROOT=${ROOT} -DLVGL26_BUILD_TESTS=OFF
                        -DFETCHCONTENT_SOURCE_DIR_LVGL=${LVGL} -DFETCHCONTENT_SOURCE_DIR_MP-UNITS=${MP_UNITS}
                        -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "the program did not configure:\n${output}")
endif()

# A header of the program misses the include: the build stops at its line.
build(OFF "parts.hpp:8: lv::list needs #include <lv/list.hpp>")
# With the include it passes.
write_header(ON)
build(ON "checking the includes of main.cpp")
# Nothing changed: the check does not run again.
if(GENERATOR MATCHES "Ninja")
  build(ON "ninja: no work to do")
else()
  build(ON "Built target app_lvgl26_includes")
endif()
message(STATUS "the build names the missing header, passes with it, and checks nothing twice")
