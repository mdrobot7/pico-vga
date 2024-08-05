set(EXECUTABLE getting-started)

add_executable(${EXECUTABLE}
    main.c
)

# pull in common dependencies
target_link_libraries(${EXECUTABLE}
    pico_stdlib
    libpicovga
    hardware_gpio
)

target_include_directories(${EXECUTABLE} PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${LIBPICOVGA_INCLUDE_DIR}"
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${EXECUTABLE})

# Enable usb output, disable uart output so we can use the pins
pico_enable_stdio_usb(${EXECUTABLE} 1)
pico_enable_stdio_uart(${EXECUTABLE} 0)

unset(EXECUTABLE)