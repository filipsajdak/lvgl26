# Project settings: lvgl26_settings() names the one settings file of a program.
#
#   add_executable(app main.cpp)
#   target_link_libraries(app PRIVATE lvgl26::lvgl26)
#   lvgl26_settings(app lvgl26.json)
#
# The file says what the program may use (allow), how strict the library is (rules) and
# what the compiler may do (limits); include/lv/config.hpp describes it, and
# docs/reference/settings.json lists every key. CMake copies the file into the build tree
# and writes a header beside the copy that embeds it. Every C++ file of the program, and of
# the libraries of this project that the program links, is compiled with that header
# included first, so no file can read other settings.
#
# A library built elsewhere with another settings file is found before the link: each
# object names the hash of its file in an undefined symbol, and a step before the link
# fails for a hash that is not the program's. Where that step does not run, the symbol
# stays undefined and the link fails.

set(LVGL26_SETTINGS_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "Where lvgl26's CMake files are")

# Writes the settings header for `file` into `directory`, and sets `hash_out` to the hash of
# the file's text. `name` is the name the messages give the file.
function(lvgl26_settings_header file name directory hash_out)
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "lvgl26_settings: there is no file ${file}")
  endif()
  configure_file("${file}" "${directory}/lvgl26.json" COPYONLY)
  file(SHA256 "${file}" full)
  string(SUBSTRING "${full}" 0 16 hash)
  set(LVGL26_SETTINGS_NAME "${name}")
  set(LVGL26_SETTINGS_HASH "${hash}")
  configure_file("${LVGL26_SETTINGS_ROOT}/settings.hpp.in" "${directory}/settings.hpp" @ONLY)
  configure_file("${LVGL26_SETTINGS_ROOT}/settings.cpp.in" "${directory}/lvgl26_settings.cpp" @ONLY)
  set(${hash_out} "${hash}" PARENT_SCOPE)
endfunction()

# The include of the settings header, for the C++ files of a target.
function(lvgl26_settings_include target header)
  target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include \"${header}\">")
endfunction()

# The library targets of this project that `target` links, directly or through others.
function(lvgl26_settings_libraries target result)
  set(seen "")
  set(todo "${target}")
  while(todo)
    list(POP_FRONT todo current)
    get_target_property(links ${current} LINK_LIBRARIES)
    get_target_property(interface_links ${current} INTERFACE_LINK_LIBRARIES)
    foreach(link IN LISTS links interface_links)
      if(TARGET "${link}" AND NOT link IN_LIST seen)
        get_target_property(real ${link} ALIASED_TARGET)
        if(real)
          set(link "${real}")
        endif()
        list(APPEND seen "${link}")
        list(APPEND todo "${link}")
      endif()
    endforeach()
  endwhile()
  set(${result} "${seen}" PARENT_SCOPE)
endfunction()

# Whether `target` is a library that this project compiles and that uses lvgl26: it reads the
# settings, so it is compiled with the program's.
function(lvgl26_settings_cxx_library target result)
  set(${result} FALSE PARENT_SCOPE)
  get_target_property(type ${target} TYPE)
  get_target_property(imported ${target} IMPORTED)
  if(imported OR NOT type MATCHES "^(STATIC|SHARED|OBJECT|MODULE)_LIBRARY$")
    return()
  endif()
  lvgl26_settings_libraries(${target} links)
  if("lvgl26" IN_LIST links)
    set(${result} TRUE PARENT_SCOPE)
  endif()
endfunction()

# Runs at the end of the directory that called lvgl26_settings(): by then the program's
# libraries are known. Compiles them with the same settings, and checks the objects before
# the link.
function(lvgl26_settings_finish target header hash name)
  lvgl26_settings_libraries(${target} libraries)
  set(archives "")
  foreach(library IN LISTS libraries)
    lvgl26_settings_cxx_library(${library} ours)
    if(ours)
      get_target_property(other ${library} LVGL26_SETTINGS_HASH)
      if(other AND NOT other STREQUAL hash)
        get_target_property(other_name ${library} LVGL26_SETTINGS_FILE)
        message(FATAL_ERROR "lvgl26_settings: the library ${library} is part of two programs with different "
                            "settings files, ${other_name} and ${name}. A library has the settings of its program.")
      endif()
      if(NOT other)
        set_target_properties(${library} PROPERTIES LVGL26_SETTINGS_HASH "${hash}" LVGL26_SETTINGS_FILE "${name}")
        lvgl26_settings_include(${library} "${header}")
      endif()
    endif()
    get_target_property(type ${library} TYPE)
    if(type STREQUAL "STATIC_LIBRARY")
      list(APPEND archives "$<TARGET_FILE:${library}>")
    endif()
  endforeach()
  get_filename_component(directory "${header}" DIRECTORY)
  set(list_file "${directory}/objects-$<CONFIG>.txt")
  file(GENERATE OUTPUT "${list_file}"
       CONTENT "$<JOIN:$<TARGET_OBJECTS:${target}>,\n>\n$<JOIN:${archives},\n>\n")
  if(CMAKE_NM)
    add_custom_command(TARGET ${target} PRE_LINK
      COMMAND "${CMAKE_COMMAND}" "-DNM=${CMAKE_NM}" "-DHASH=${hash}" "-DNAME=${name}" "-DLIST=${list_file}"
              -P "${LVGL26_SETTINGS_ROOT}/settings_scan.cmake"
      COMMENT "Checking that every object of ${target} has the settings of ${name}"
      VERBATIM)
  endif()
endfunction()

# Names the settings file of the program `target`. A program has one.
function(lvgl26_settings target file)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "lvgl26_settings: ${target} is not a target. Call it after add_executable(${target} ...).")
  endif()
  get_target_property(known ${target} LVGL26_SETTINGS_FILE)
  if(known)
    message(FATAL_ERROR "lvgl26_settings: ${target} has a settings file already, ${known}. A program has one.")
  endif()
  get_filename_component(path "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  set(directory "${CMAKE_CURRENT_BINARY_DIR}/lvgl26-settings/${target}")
  lvgl26_settings_header("${path}" "${file}" "${directory}" hash)
  set(header "${directory}/settings.hpp")
  set_target_properties(${target} PROPERTIES LVGL26_SETTINGS_HASH "${hash}" LVGL26_SETTINGS_FILE "${file}")
  lvgl26_settings_include(${target} "${header}")
  target_sources(${target} PRIVATE "${directory}/lvgl26_settings.cpp")
  cmake_language(EVAL CODE
    "cmake_language(DEFER CALL lvgl26_settings_finish [[${target}]] [[${header}]] [[${hash}]] [[${file}]])")
endfunction()
