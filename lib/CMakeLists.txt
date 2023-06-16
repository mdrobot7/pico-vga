# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(../pico_sdk_import.cmake)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_compile_options(-O3 -DNDEBUG) # compiler optimization, disable sdk function parameter checking

add_library(PICO_VGA_LIB STATIC
    audio.c
    controller.c
    draw-2d.c
    draw-3d.c
    init.c
    init-display.c
    peripheral-mode.c
    render/render.c
    render/render-2d.c
    sd.c
    stats.c
    utils-internal.c
    utils.c
)
target_include_directories(PICO_VGA_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Create C header file with the name <pio program>.pio.h
# All 3 calls are REQUIRED, otherwise it doesn't build for some reason.
pico_generate_pio_header(PICO_VGA_LIB 
        ${CMAKE_CURRENT_SOURCE_DIR}/color.pio
)

target_link_libraries(PICO_VGA_LIB 
    pico_stdlib
    hardware_gpio
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_pwm
    hardware_i2c
    hardware_spi
    pico_multicore
    pico_time
)