# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(../pico_sdk_import.cmake)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_library(GAME STATIC
    game.c
    sprites.c
)

target_include_directories(GAME PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(GAME 
    PICO_VGA_LIB
    pico_stdlib
    hardware_gpio
)