# The slice for a microcontroller. Built by the preset arm-m4 (see cmake/arm/toolchain.cmake)
# and run in QEMU:
#   qemu-system-arm -M mps2-an386 -cpu cortex-m4 -nographic \
#       -semihosting-config enable=on,target=native -kernel build/arm-m4/examples/mcu/slice.elf
add_executable(slice.elf slice.cpp ${PROJECT_SOURCE_DIR}/cmake/arm/startup.c)
target_link_libraries(slice.elf PRIVATE lvgl26::lvgl26)
target_compile_options(slice.elf PRIVATE -Os)
target_link_options(slice.elf PRIVATE -T ${PROJECT_SOURCE_DIR}/cmake/arm/mps2-an386.ld
                    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/slice.map)

# The same screen as plain LVGL C, the baseline the slice is measured against
# (docs/targets.md). It links LVGL with the same configuration, and no lvgl26. It lives in
# tools/: an example calls no C API.
add_executable(slice_c.elf ${PROJECT_SOURCE_DIR}/tools/arm_slice_c.c ${PROJECT_SOURCE_DIR}/cmake/arm/startup.c)
target_link_libraries(slice_c.elf PRIVATE lvgl::lvgl)
target_compile_options(slice_c.elf PRIVATE -Os)
target_link_options(slice_c.elf PRIVATE -T ${PROJECT_SOURCE_DIR}/cmake/arm/mps2-an386.ld
                    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/slice_c.map)

# The stack of one run-time set, for tools/stack_budget.py (run by tools/arm-run.sh). It is
# compiled only, as the slice is, never linked.
add_library(set_stack_probe OBJECT set_probe.cpp)
target_link_libraries(set_stack_probe PRIVATE lvgl26::lvgl26)
target_compile_options(set_stack_probe PRIVATE -Os -fstack-usage)
add_dependencies(slice.elf set_stack_probe)
