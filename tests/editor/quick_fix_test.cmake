# The test editor_quick_fix: in a project that turns editor support on, clangd builds its
# background index of lvgl26 and offers the quick fix that adds a missing header.
#
#   cmake -DSOURCE=<lvgl26> -DWORK=<empty directory> -DGENERATOR=<generator>
#         -DOPTIONS=<list of -D options for the project> -DCLANGD=<clangd> -DPYTHON=<python3>
#         -P quick_fix_test.cmake
#
# It takes minutes, most of them clangd indexing lvgl26, so it runs every night (label nightly).

file(REMOVE_RECURSE ${WORK})
file(COPY ${SOURCE}/tests/editor/consumer/CMakeLists.txt ${SOURCE}/examples/theme/tokens.json
     DESTINATION ${WORK}/project)
file(COPY_FILE ${SOURCE}/examples/theme/main.cpp ${WORK}/project/theme.cpp)
# The program of the project uses lv::scaled_font without its header: a class that is no
# template, so only clangd's index knows where it is.
file(WRITE ${WORK}/project/main.cpp
     "#include <lv/lv.hpp>\n\nlv::scaled_font* font = nullptr;\n\nint main() { return font == nullptr ? 0 : 1; }\n")

execute_process(COMMAND ${CMAKE_COMMAND} -S ${WORK}/project -B ${WORK}/build -G ${GENERATOR} ${OPTIONS}
                RESULT_VARIABLE failed OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(failed)
  message(FATAL_ERROR "editor_quick_fix: the project does not configure.\n${output}")
endif()

execute_process(COMMAND ${PYTHON} ${SOURCE}/tools/editor/quick_fix.py --clangd ${CLANGD} --project ${WORK}/project
                        --file main.cpp --start theme.cpp --expect "Include <lv/scaled_font.hpp> for symbol lv::scaled_font"
                RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR "editor_quick_fix: clangd offers no quick fix for the missing header.")
endif()
