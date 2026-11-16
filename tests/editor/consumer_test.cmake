# The test editor_support: a project turns editor support on with one line.
#
#   cmake -DSOURCE=<lvgl26> -DWORK=<empty directory> -DGENERATOR=<generator>
#         -DOPTIONS=<list of -D options for the project> [-DCLANGD=<clangd>] -P consumer_test.cmake
#
# It configures a copy of tests/editor/consumer, with the quick start and the theme
# example as its programs. It does not build them: clangd reads sources, not programs.

# expect("what went wrong" <condition as if() takes it>)
function(expect message)
  if(NOT (${ARGN}))
    message(FATAL_ERROR "editor_support: ${message}")
  endif()
endfunction()

function(configure)
  execute_process(COMMAND ${CMAKE_COMMAND} -S ${WORK}/project -B ${WORK}/build -G ${GENERATOR} ${OPTIONS}
                  RESULT_VARIABLE failed OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(failed)
    message(FATAL_ERROR "editor_support: the project does not configure.\n${output}")
  endif()
  set(output "${output}" PARENT_SCOPE)
endfunction()

# clangd reads a file as an editor would, with the project's .clangd.
function(clangd_reads file)
  execute_process(COMMAND ${CLANGD} --check=${WORK}/project/${file} --log=error
                  WORKING_DIRECTORY ${WORK}/project
                  RESULT_VARIABLE failed OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(failed)
    message(FATAL_ERROR "editor_support: clangd reports errors in ${file}.\n${output}")
  endif()
endfunction()

file(REMOVE_RECURSE ${WORK})
file(COPY ${SOURCE}/tests/editor/consumer/CMakeLists.txt ${SOURCE}/examples/quick_start/main.cpp
          ${SOURCE}/examples/theme/tokens.json
     DESTINATION ${WORK}/project)
file(COPY_FILE ${SOURCE}/examples/theme/main.cpp ${WORK}/project/theme.cpp)

configure()
set(stubs ${WORK}/build/lvgl26-editor)
expect("no .clangd was written into the project" EXISTS ${WORK}/project/.clangd)
expect("no stubs were written into the build directory" EXISTS ${stubs}/lv/generated.hpp)
file(READ ${WORK}/build/compile_commands.json commands)
expect("compile_commands.json does not have main.cpp" commands MATCHES "project/main.cpp")
expect("compile_commands.json does not have theme.cpp" commands MATCHES "project/theme.cpp")
file(READ ${stubs}/overlay.yaml overlay)
expect("the token file of the project was not found" overlay MATCHES "project/tokens.json")
file(READ ${WORK}/project/.clangd written)
expect(".clangd does not lead to the stubs" written MATCHES "ivfsoverlay")

if(CLANGD)
  clangd_reads(main.cpp)
  clangd_reads(theme.cpp)
endif()

# A .clangd that is there already is never changed.
file(WRITE ${WORK}/project/.clangd "# mine\n")
configure()
file(READ ${WORK}/project/.clangd kept)
expect("a .clangd that was there was overwritten" kept STREQUAL "# mine\n")
expect("the message does not say what to add to the .clangd" output MATCHES "clangd.example")
expect("what to add to the .clangd was not written" EXISTS ${stubs}/clangd.example)
