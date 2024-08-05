# Installation
This guide assumes you have the Pi Pico SDK and toolchain set up on your operating system. If not, follow the [Pi Pico setup guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) for your OS.

## Building the Examples
Examples are found under `/examples`, each one builds into its own binary that you can drop onto a Pico. The following will build all examples.
```
mkdir build
cd build
cmake ..
make -j4
```
Each example can then be dropped onto a pico with USB or with a debugger.

## Integrating Into Your Own Projects
This project is meant as a library for larger projects/programs. The library source code is in `/src`, and the main include header is in `/inc`. Use `#include "pico-vga.h"` at the top of your C source, and add the following to your `CMakeLists.txt`:
```
add_subdirectory(path/to/pico-vga/src)
target_include_directories(YOUR_EXECUTABLE_NAME PUBLIC
    "${PROJECT_BINARY_DIR}"
    "path/to/pico-vga/inc"
)
target_link_libraries(YOUR_EXECUTABLE_NAME
    libpicovga
)
```