# Include build functions from Pico SDK
include(../../pico-sdk/external/pico_sdk_import.cmake)
include(../../pico-sdk/tools/CMakeLists.txt)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_library(PICO_VGA_SDK STATIC
    sdk.c
)
target_include_directories(PICO_VGA_SDK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Create C header file with the name <pio program>.pio.h
# All 3 calls are REQUIRED, otherwise it doesn't build for some reason.
pico_generate_pio_header(PICO_VGA_SDK  
        ${CMAKE_CURRENT_SOURCE_DIR}/color.pio
)
pico_generate_pio_header(PICO_VGA_SDK  
        ${CMAKE_CURRENT_SOURCE_DIR}/hsync.pio
)
pico_generate_pio_header(PICO_VGA_SDK  
        ${CMAKE_CURRENT_SOURCE_DIR}/vsync.pio
)

target_link_libraries(PICO_VGA_SDK 
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_gpio
    pico_multicore
    pico_malloc
)