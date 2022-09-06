## Configuration
You **must** clone this repository ALONGSIDE the Pi Pico SDK, otherwise it WILL NOT BUILD! The file tree must look like this:

/
- pico-sdk/
- pico-vga/

### Alternative Configuration Instructions
If you absolutely cannot clone it alongside the SDK, then edit ```CMakeLists.txt``` lines 8 and 9 to match the path to those files within the SDK.