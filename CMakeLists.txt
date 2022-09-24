# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include(../pico-sdk/external/pico_sdk_import.cmake)
include(../pico-sdk/tools/CMakeLists.txt)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(pico-vga C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source files
FILE(GLOB gameSources ./game/*.c)

add_executable(${PROJECT_NAME} 
    main.c
    ${gameSources} # NOTE! This line is dangerous. It tells CMake to build every .c file in /game/. If build
                   #       happen, it's likely this line.
)

# Create C header file with the name <pio program>.pio.h
# All 3 calls are REQUIRED, otherwise it doesn't build for some reason.
pico_generate_pio_header(${PROJECT_NAME}  
        ${CMAKE_CURRENT_LIST_DIR}/color.pio
)
pico_generate_pio_header(${PROJECT_NAME}  
        ${CMAKE_CURRENT_LIST_DIR}/hsync.pio
)
pico_generate_pio_header(${PROJECT_NAME}  
        ${CMAKE_CURRENT_LIST_DIR}/vsync.pio
)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_gpio
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})