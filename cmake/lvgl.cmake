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
