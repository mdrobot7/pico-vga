# Include build functions from Pico SDK
include(../../pico-sdk/external/pico_sdk_import.cmake)
include(../../pico-sdk/tools/CMakeLists.txt)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_library(PICO_VGA_LIB STATIC
    audio.c
    controller.c
    draw-2d.c
    draw-3d.c
    init.c
    initDisplay.c
    peripheralMode.c
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
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_gpio
    hardware_i2c
    hardware_pwm
    pico_multicore
    pico_malloc
)