# Before a program is linked: fails if an object or archive names the mark of other settings.
#   cmake -DNM=<nm> -DHASH=<hash of the program's file> -DNAME=<its name> -DLIST=<file of paths> -P settings_scan.cmake
file(STRINGS "${LIST}" paths)
foreach(path IN LISTS paths)
  if(path STREQUAL "" OR NOT EXISTS "${path}")
    continue()
  endif()
  execute_process(COMMAND "${NM}" "${path}" OUTPUT_VARIABLE symbols ERROR_QUIET RESULT_VARIABLE failed)
  if(failed)
    continue()
  endif()
  string(REGEX MATCHALL "lvgl26_settings_[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]" marks "${symbols}")
  list(REMOVE_DUPLICATES marks)
  list(REMOVE_ITEM marks "lvgl26_settings_${HASH}")
  if(marks)
    list(GET marks 0 other)
    # A notice is printed as one line, so the message can be searched for.
    message(NOTICE "lv: ${path} was compiled with other settings (${other}) than this program, whose "
                   "settings file is ${NAME} (lvgl26_settings_${HASH}). Compile it with the same settings file.")
    cmake_language(EXIT 1)
  endif()
endforeach()
