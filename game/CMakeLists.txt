# Include build functions from Pico SDK
include(../../pico-sdk/external/pico_sdk_import.cmake)
include(../../pico-sdk/tools/CMakeLists.txt)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_library(GAME STATIC
    game.c
    sprites.c
)

target_include_directories(GAME PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(GAME 
    PICO_VGA_SDK
    pico_stdlib
    hardware_gpio
)