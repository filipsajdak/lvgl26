# Adds a pinned LVGL as the target lvgl::lvgl.
# LVGL26_LVGL_REF is a tag or a full commit sha.
# To build from a local checkout, set FETCHCONTENT_SOURCE_DIR_LVGL.
include(FetchContent)

set(LVGL26_LVGL_REF "v9.6.0" CACHE STRING "LVGL tag or commit sha to build against")

# LVGL is configured from compile definitions, not from an lv_conf.h.
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lvgl
  URL "https://github.com/lvgl/lvgl/archive/${LVGL26_LVGL_REF}.tar.gz"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(lvgl)

# LVGL's headers are system headers to us: our warning flags do not apply to them.
set_target_properties(lvgl PROPERTIES SYSTEM TRUE)

# lv::write_png draws a widget through LVGL's snapshot. A program that never calls
# it does not link that code.
target_compile_definitions(lvgl PUBLIC LV_USE_SNAPSHOT=1)

# LVGL's own heap. LVGL's default is 64 KB: enough for a small screen on a
# microcontroller, too little for a picture of a whole screen.
set(LVGL26_LVGL_HEAP_BYTES "" CACHE STRING "The size of LVGL's own heap in bytes; empty keeps LVGL's default")
if(LVGL26_LVGL_HEAP_BYTES)
  target_compile_definitions(lvgl PUBLIC LV_MEM_SIZE=${LVGL26_LVGL_HEAP_BYTES})
endif()

# Any other LVGL setting, as LVGL names it: -DLVGL26_LVGL_DEFINITIONS="LV_USE_LOG=1;LV_COLOR_DEPTH=32".
set(LVGL26_LVGL_DEFINITIONS "" CACHE STRING "More compile definitions for LVGL, as LVGL names them")
if(LVGL26_LVGL_DEFINITIONS)
  target_compile_definitions(lvgl PUBLIC ${LVGL26_LVGL_DEFINITIONS})
endif()

# The tests render headless and compare screenshots.
if(LVGL26_BUILD_TESTS)
  target_compile_definitions(lvgl PUBLIC
    LV_USE_TEST=1 LV_USE_TEST_SCREENSHOT_COMPARE=1 LV_USE_LODEPNG=1
    LV_USE_LOG=1 LV_LOG_PRINTF=1 LV_LOG_LEVEL=LV_LOG_LEVEL_WARN
    LV_MEM_SIZE=4194304
    LV_USE_FS_STDIO=1 LV_FS_STDIO_LETTER=65
    LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE=$<BOOL:${LVGL26_UPDATE_REFERENCES}>)
endif()

# mp-units provides the units of application values in the tests. lvgl26's headers
# do not include it: they accept any quantity of its shape.
if(LVGL26_BUILD_TESTS)
  set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)
  set(MP_UNITS_API_STD_FORMAT TRUE CACHE STRING "" FORCE)
  set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(mp-units
    URL "https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR src
    EXCLUDE_FROM_ALL
    SYSTEM)
  FetchContent_MakeAvailable(mp-units)
endif()
