# Editor support: lvgl26_editor_support() lets clangd, the language server behind most
# editors, read a project that uses lvgl26.
#
#   FetchContent_MakeAvailable(lvgl26)
#   add_executable(app main.cpp)
#   target_link_libraries(app PRIVATE lvgl26::lvgl26)
#   lvgl26_editor_support()
#
# clangd cannot parse lvgl26's headers: they use C++26 reflection, which only GCC 16
# has. So tools/editor/generate.py writes stub headers into
# ${CMAKE_BINARY_DIR}/lvgl26-editor: the same public API as plain C++. A file `.clangd`
# in the project's source directory tells clangd to read them in place of the real
# headers. GCC still builds the program from the real ones.
#
#   lvgl26_editor_support([TOKENS file...])
#
# TOKENS names design token files that are not under the project's source directory.
# The ones under it are found by looking: see tools/editor/generate.py.

set(LVGL26_EDITOR_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "Where lvgl26 is")

# The reference data of the LVGL in use: docs/reference/lvgl-<version>.json.
function(lvgl26_editor_reference result)
  set(directory "${LVGL26_EDITOR_ROOT}/docs/reference")
  if(LVGL26_LVGL_REF MATCHES "^v[0-9.]+$" AND EXISTS "${directory}/lvgl-${LVGL26_LVGL_REF}.json")
    set(${result} "${directory}/lvgl-${LVGL26_LVGL_REF}.json" PARENT_SCOPE)
  elseif(LVGL26_LVGL_REF MATCHES "^v9")
    message(STATUS "lvgl26 editor support: no reference data for LVGL ${LVGL26_LVGL_REF}; using v9.6.0")
    set(${result} "${directory}/lvgl-v9.6.0.json" PARENT_SCOPE)
  else()
    set(${result} "${directory}/lvgl-v10-pin.json" PARENT_SCOPE)
  endif()
endfunction()

# A target made before CMAKE_EXPORT_COMPILE_COMMANDS was set is left out of
# compile_commands.json, so every target under `directory` is told to be in it.
function(lvgl26_editor_export_commands directory)
  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    get_target_property(type ${target} TYPE)
    if(NOT type STREQUAL "INTERFACE_LIBRARY" AND NOT type STREQUAL "UTILITY")
      set_target_properties(${target} PROPERTIES EXPORT_COMPILE_COMMANDS ON)
    endif()
  endforeach()
  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    lvgl26_editor_export_commands("${subdirectory}")
  endforeach()
endfunction()

# The text of the file .clangd.
function(lvgl26_editor_clangd result stubs)
  # clangd takes the name of the compiler from the build's command, which is GCC. Named
  # as Clang, it looks for the standard library the way Clang does.
  find_program(LVGL26_EDITOR_CLANGXX NAMES clang++ HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin
               DOC "The clang++ that clangd is told to act as")
  set(compiler clang++)
  if(LVGL26_EDITOR_CLANGXX)
    set(compiler "${LVGL26_EDITOR_CLANGXX}")
  endif()
  string(CONCAT text
    "# Written by lvgl26_editor_support() (cmake/editor.cmake of lvgl26). It holds paths of\n"
    "# this machine: keep it out of version control. Delete it and run CMake to get a new one.\n"
    "CompileFlags:\n"
    "  CompilationDatabase: \"${CMAKE_BINARY_DIR}\"\n"
    "  Compiler: \"${compiler}\"\n"
    "  # The overlay leads <lv/...> and the design token files to the stubs.\n"
    "  Add:\n"
    "    - \"-ivfsoverlay\"\n"
    "    - \"${stubs}/overlay.yaml\"\n"
    "    - \"-idirafter\"\n"
    "    - \"${stubs}/fallback\"\n"
    "    - \"-Wno-c23-extensions\"\n"
    "    - \"-Werror=reorder-init-list\"\n"
    "  # What only GCC understands.\n"
    "  Remove:\n"
    "    - \"-freflection\"\n"
    "    - \"-fconstexpr-ops-limit=*\"\n"
    "    - \"-fanalyzer\"\n"
    "    - \"-fno-analyzer\"\n"
    "    - \"-fmodules-ts\"\n"
    "    - \"-fmodule-mapper=*\"\n"
    "    - \"-fdeps-format=*\"\n"
    "    - \"-W*\"\n")
  set(${result} "${text}" PARENT_SCOPE)
endfunction()

function(lvgl26_editor_support)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "" "TOKENS")
  find_package(Python3 COMPONENTS Interpreter)
  if(NOT Python3_FOUND)
    message(WARNING "lvgl26 editor support: Python 3 was not found, so no stubs were written.")
    return()
  endif()

  set(stubs "${CMAKE_BINARY_DIR}/lvgl26-editor")
  lvgl26_editor_reference(reference)
  set(tokens)
  foreach(file IN LISTS arg_TOKENS)
    cmake_path(ABSOLUTE_PATH file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
    list(APPEND tokens "${file}")
  endforeach()
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${LVGL26_EDITOR_ROOT}/tools/editor/generate.py"
            --reference "${reference}" --out "${stubs}" --real-include "${LVGL26_EDITOR_ROOT}/include"
            --scan "${CMAKE_SOURCE_DIR}" --skip "${CMAKE_BINARY_DIR}" --tokens ${tokens}
    RESULT_VARIABLE failed OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(failed)
    message(FATAL_ERROR "lvgl26 editor support: the stubs could not be written.\n${output}")
  endif()

  # A changed token file has new members: CMake runs again, and with it the generator.
  file(STRINGS "${stubs}/token_files.txt" found)
  set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${found})

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Write compile_commands.json, which clangd reads" FORCE)
  lvgl26_editor_export_commands("${CMAKE_SOURCE_DIR}")

  lvgl26_editor_clangd(text "${stubs}")
  set(file "${CMAKE_SOURCE_DIR}/.clangd")
  if(NOT EXISTS "${file}")
    file(WRITE "${file}" "${text}")
    message(STATUS "lvgl26 editor support: wrote ${file}; the stubs are in ${stubs}")
    return()
  endif()
  file(READ "${file}" present)
  if(NOT present STREQUAL text)
    file(WRITE "${stubs}/clangd.example" "${text}")
    message(STATUS "lvgl26 editor support: ${file} exists and was left as it is. "
                   "What lvgl26 needs in it is in ${stubs}/clangd.example: copy its CompileFlags.")
  endif()
endfunction()
