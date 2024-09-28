set(EXECUTABLE getting-started)

add_executable(${EXECUTABLE}
    main.c
)

# pico-vga definitions
# The *total* amount of memory that the entire Pico-VGA library is allowed to use (framebuffer, render elements, etc).
# Try to maximize this, since the more memory the library is given the better it will perform (recommended: 256kB)
target_compile_definitions(${EXECUTABLE} PUBLIC PV_FRAMEBUFFER_BYTES=200000)
# Whether the board should act in peripheral mode
target_compile_definitions(${EXECUTABLE} PUBLIC PV_PERIPHERAL_MODE=false)

# pull in common dependencies
target_link_libraries(${EXECUTABLE}
    pico_stdlib
    libpicovga
    hardware_pio
    hardware_sync
)

target_include_directories(${EXECUTABLE} PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${LIBPICOVGA_INCLUDE_DIR}"
)

# Print out flash/RAM usage after build finished successfully
add_custom_command(
  TARGET ${EXECUTABLE} POST_BUILD
  COMMAND arm-none-eabi-size "$<TARGET_FILE:${EXECUTABLE}>"
  VERBATIM)

# create map/bin/hex file etc.
pico_add_extra_outputs(${EXECUTABLE})

# Enable usb output, disable uart output so we can use the pins
pico_enable_stdio_usb(${EXECUTABLE} 1)
pico_enable_stdio_uart(${EXECUTABLE} 0)

unset(EXECUTABLE)