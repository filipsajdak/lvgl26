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

# The pinned release is checked against the hash of its archive. Another tag or a sha is
# the user's choice and has no hash here.
set(LVGL26_LVGL_V960_SHA256 "b20ee3acc1bba13c62d854f9ebd62e4c51e0b443b1e0225892e86442defa84df")
if(LVGL26_LVGL_REF STREQUAL "v9.6.0")
  set(LVGL26_LVGL_HASH URL_HASH SHA256=${LVGL26_LVGL_V960_SHA256})
else()
  set(LVGL26_LVGL_HASH "")
endif()

FetchContent_Declare(lvgl
  URL "https://github.com/lvgl/lvgl/archive/${LVGL26_LVGL_REF}.tar.gz"
  ${LVGL26_LVGL_HASH}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(lvgl)

# LVGL's headers are system headers to us: our warning flags do not apply to them.
set_target_properties(lvgl PROPERTIES SYSTEM TRUE)

# lv::write_png draws a widget through LVGL's snapshot. A program that never calls
# it does not link that code.
target_compile_definitions(lvgl PUBLIC LV_USE_SNAPSHOT=1)

# LVGL's own heap. LVGL's default is 64 KB: enough for a small screen on a
# microcontroller, too little for a picture of a whole screen. So a build for the
# machine you work on gets 1 MB, and a cross build keeps LVGL's default.
set(LVGL26_LVGL_HEAP_BYTES "" CACHE STRING
    "The size of LVGL's own heap in bytes; empty means 1 MB on a hosted build and LVGL's default on a cross build")
set(LVGL26_HEAP "${LVGL26_LVGL_HEAP_BYTES}")
if(NOT LVGL26_HEAP AND LVGL26_BUILD_TESTS)
  set(LVGL26_HEAP 4194304)  # the tests draw screens of every size
elseif(NOT LVGL26_HEAP AND NOT CMAKE_CROSSCOMPILING)
  set(LVGL26_HEAP 1048576)
endif()
if(LVGL26_HEAP)
  target_compile_definitions(lvgl PUBLIC LV_MEM_SIZE=${LVGL26_HEAP})
endif()

# Fonts that the device draws from a TrueType file (lv::scaled_font). LVGL's TinyTTF
# adds code and uses the heap, so a program asks for it.
if(LVGL26_TTF OR LVGL26_BUILD_TESTS)
  target_compile_definitions(lvgl PUBLIC LV_USE_TINY_TTF=1)
endif()

# Pictures kept as files and decoded on the device (lv::encoded_image). Each decoder
# adds code and uses the heap, so a program asks for it. LVGL's JPEG decoder reads a
# file in memory through its memory file system, which gets the drive letter M. The
# tests compare screenshots with the PNG decoder.
if(LVGL26_PNG OR LVGL26_BUILD_TESTS)
  target_compile_definitions(lvgl PUBLIC LV_USE_LODEPNG=1)
endif()
if(LVGL26_JPEG OR LVGL26_BUILD_TESTS)
  target_compile_definitions(lvgl PUBLIC LV_USE_TJPGD=1 LV_USE_FS_MEMFS=1 LV_FS_MEMFS_LETTER=77)
endif()

# Any other LVGL setting, as LVGL names it: -DLVGL26_LVGL_DEFINITIONS="LV_USE_LOG=1;LV_COLOR_DEPTH=32".
set(LVGL26_LVGL_DEFINITIONS "" CACHE STRING "More compile definitions for LVGL, as LVGL names them")
if(LVGL26_LVGL_DEFINITIONS)
  target_compile_definitions(lvgl PUBLIC ${LVGL26_LVGL_DEFINITIONS})
endif()

# The tests render headless and compare screenshots.
if(LVGL26_BUILD_TESTS)
  target_compile_definitions(lvgl PUBLIC
    LV_USE_TEST=1 LV_USE_TEST_SCREENSHOT_COMPARE=1
    LV_USE_LOG=1 LV_LOG_PRINTF=1 LV_LOG_LEVEL=LV_LOG_LEVEL_WARN
    LV_USE_FS_STDIO=1 LV_FS_STDIO_LETTER=65
    LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE=$<BOOL:${LVGL26_UPDATE_REFERENCES}>)
endif()

# mp-units gives application values their units (120 * km / h). lvgl26's headers do
# not include it: they accept any quantity of its shape. LVGL26_UNITS fetches it and
# links it to lvgl26, so a project needs no lines of its own for it. The tests fetch
# it and link it themselves, which keeps the headers honest.
if(LVGL26_UNITS OR LVGL26_BUILD_TESTS)
  set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)
  set(MP_UNITS_API_STD_FORMAT TRUE CACHE STRING "" FORCE)
  set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(mp-units
    URL "https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz"
    URL_HASH SHA256=a6bd48bee699f11f0ed5b04b8c5006d15f76e6d898e058db3880554d2e47a400
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR src
    EXCLUDE_FROM_ALL
    SYSTEM)
  FetchContent_MakeAvailable(mp-units)
endif()
