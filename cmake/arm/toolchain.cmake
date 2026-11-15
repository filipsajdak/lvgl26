# Cross-compiles for an Arm Cortex-M4 with arm-none-eabi GCC 16 and the libraries that
# tools/arm-toolchain.sh builds. Used by the preset arm-m4:
#   tools/arm-toolchain.sh build/_tools/arm
#   cmake --preset arm-m4 && cmake --build --preset arm-m4
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Where tools/arm-toolchain.sh put newlib and libstdc++.
if(NOT DEFINED LVGL26_ARM_SYSROOT)
  if(DEFINED ENV{LVGL26_ARM_SYSROOT})
    set(LVGL26_ARM_SYSROOT "$ENV{LVGL26_ARM_SYSROOT}")
  else()
    set(LVGL26_ARM_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../../build/_tools/arm/arm-none-eabi")
  endif()
endif()
get_filename_component(LVGL26_ARM_SYSROOT "${LVGL26_ARM_SYSROOT}" ABSOLUTE)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LVGL26_ARM_SYSROOT)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
# A bare-metal program cannot be linked without start-up code: try compiling only.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(arm_cpu "-mcpu=cortex-m4 -mthumb")
set(arm_c "-isystem ${LVGL26_ARM_SYSROOT}/include -B${LVGL26_ARM_SYSROOT}/lib/")
set(arm_cxx "-isystem ${LVGL26_ARM_SYSROOT}/include/c++/16.2.0 -isystem ${LVGL26_ARM_SYSROOT}/include/c++/16.2.0/arm-none-eabi")
set(CMAKE_C_FLAGS_INIT "${arm_cpu} ${arm_c} -ffunction-sections -fdata-sections")
# The order matters: the C++ headers come before the C ones.
set(CMAKE_CXX_FLAGS_INIT "${arm_cpu} ${arm_cxx} ${arm_c} -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${LVGL26_ARM_SYSROOT}/lib --specs=rdimon.specs -nostartfiles -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
