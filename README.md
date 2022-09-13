# Reqirements
This code was developed on Windows 11 using the Raspberry Pi Pico official C++ SDK and Visual Studio Code. It is intended to work in that environment, and no guarantees are made that it will build cleanly with no extra configuration on Linux or MacOS. However, the compiled `.uf2` file will work regardless of the OS used.

## Software Dependencies
- Raspberry Pi Pico C++ SDK, and its dependencies (see official setup instructions)
- CMake
- Visual Studio Code
- Python 3.10+
- OpenCV-Python 4.6.0+
- Numpy

## Hardware Requirements
- A Raspberry Pi Pico, obviously!
- A monitor with VGA input
- A breadboard, or a piece of protoboard/perfboard if doing a complete assembly

Since VGA is a true analog signal, it is not enough to cut up a sacrificial cable and tie it onto the Pico's pins. Some extra hardware is required to make color work, which is why a breadboard is highly recommended.

# Configuration
You **must** clone this repository ALONGSIDE the Pi Pico SDK, otherwise it WILL NOT BUILD! The file tree must look like this:

/
- pico-sdk/
- pico-vga/

### Alternative Configuration Instructions
If you absolutely cannot clone it alongside the SDK, then edit ```CMakeLists.txt``` lines 8 and 9 to match the path to those files within the SDK.