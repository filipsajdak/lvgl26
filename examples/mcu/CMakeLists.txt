# The slice for a microcontroller. Built by the preset arm-m4 (see cmake/arm/toolchain.cmake)
# and run in QEMU:
#   qemu-system-arm -M mps2-an386 -cpu cortex-m4 -nographic \
#       -semihosting-config enable=on,target=native -kernel build/arm-m4/examples/mcu/slice.elf
add_executable(slice.elf slice.cpp ${PROJECT_SOURCE_DIR}/cmake/arm/startup.c)
target_link_libraries(slice.elf PRIVATE lvgl26::lvgl26)
target_compile_options(slice.elf PRIVATE -Os)
target_link_options(slice.elf PRIVATE -T ${PROJECT_SOURCE_DIR}/cmake/arm/mps2-an386.ld
                    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/slice.map)
