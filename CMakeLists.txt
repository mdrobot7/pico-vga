# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(pico-vga-examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_compile_options(-O3 -DNDEBUG -Wall -Wextra)

# Add in the pico-vga library
add_subdirectory(src)
set(LIBPICOVGA_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/inc")

add_subdirectory(examples/getting-started)