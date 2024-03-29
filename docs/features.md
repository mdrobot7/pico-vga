# Pico-VGA Features

- [x] Render a 400x300 frame @60Hz with 8 bit color over VGA with no artifacts, and be able to modify it.

## Renderer
- [x] Make the DMA callback for the renderer the NMI for core 1
- [x] Be able to reset the resolution on the fly with dynamic allocation for the frame and other arrays
- [ ] Add line interpolation for higher resolutions (640x480, 800x600, 1024x768) by setting a memory usage cap (add #define'd recommended memory values)
- [ ] Make the color state machine initial delay a parameter
- [x] Add alternative hsync and vsync PIO files for 640x480 resolution, and sys_clk reconfiguration
- [x] Add 1024x768 OC mode

## Renderer v2
- [ ] Make a renderer that doesn't waste cycles overwriting data in the frame array (i.e. filling a rectangle in, then putting something else in the middle of it, making a lot of the filling operation wasted) -- see https://en.wikipedia.org/wiki/Scanline_rendering

## 2D SDK
- [x] Make a render queue
- [x] Make an automatic and manual rendering mode
- [x] Make modifier functions for render queue items
- [x] Draw individual pixels
- [ ] Draw lines
  - [x] Draw basic lines using floating point math
  - [x] Draw lines using only integer math (Bresenham's algorithm)
  - [ ] Draw anti-aliased lines
  - [ ] Draw lines with different thicknesses
- [ ] Draw rectangles
  - [x] Draw basic rectangles
  - [ ] Draw with different thicknesses
- [ ] Be able to fill the screen
  - [x] Be able to fill the screen with a solid color
  - [ ] Be able to fill the screen with a sprite/vector drawing
- [x] Draw filled rectangles
- [ ] Draw triangles
  - [x] Draw basic triangles
  - [x] Draw triangles using only integer math
  - [ ] Draw antialiased triangles
  - [ ] Draw with different thicknesses
- [ ] Draw filled triangles with only integer math
- [ ] Draw circles
  - [x] Draw basic circles using floating point math
  - [ ] Draw circles using only integer math (Midpoint circle algorithm)
  - [ ] Draw antialiased circles
  - [ ] Draw with different thicknesses
- [ ] Draw filled circles
  - [x] Draw basic filled circles
  - [ ] Draw antialiased filled circles
- [ ] Draw sprites
  - [ ] Draw bitmaps (two-color, smaller size)
  - [ ] Draw basic sprites
  - [ ] Add a flag that keeps the sprite's shape but makes it all one color
  - [ ] Make sprites resizable
- [ ] Draw SVGs (lists of vectors)
  - [ ] Make an SVG parser to convert SVGs to a custom type or something
  - [ ] Add types for either a new struct or renderQueueItem to represent more SVG elements
- [ ] Draw text
  - [ ] Draw text from the CP437 font with a string passed to a function
  - [ ] Allow the text background to be changed
  - [ ] Make the text resizable
  - [x] Allow the user to change the font
  - [ ] Add antialiasing
  - [ ] Make a TTF (or other common font type) to header file converter (python)
- [ ] Draw a set of points connected in order by lines
- [ ] Fill a set of points connected by lines
- [ ] Add rotation for all elements
- [ ] Add "light" render element -- acts as a light source for a certain radius ("brightens" all pixels)

## 3D SDK
- [ ] Parse .obj (Wavefront) files
- [ ] Parse .stl files?
- [ ] Write 3D render algorithm
- [ ] Add rotation, scaling values

## Audio SDK
- [x] Make a struct to hold audio data for a particular song (name, pointer to array of notes/lengths, array length)
- [x] Make an RTTTL parser to output song data in the struct form
- [x] Make audio play through PWM peripheral, make interrupts on core 1 change frequencies/note lengths
- [x] playX() functions
  - [x] playNote(), which plays an individual note for a certain length
  - [x] Beep/Boop noises
  - [x] Static
  - [x] Snare hit, hi hat, kick drum (all the noises supported on NES, SNES)

## SD Card
- [ ] Read text files from the SD card
- [ ] Parse text files from the SD card (SVGs, RTTTL files, OBJs)
- [ ] Write persistent data to the SD card
- [ ] Read large images (>150kB) from the SD card and output them
- [ ] Read traditional audio files from the SD card (WAV, MIDI)

## Peripheral Mode
- [ ] Make docs for how to control the pico via SPI (what bits correspond to what)
- [ ] Peripheral Mode will be pre-determined at compile-time via a #define. Make a program to drive it -- read the SPI messages, parse them, and add things to the render queue
- [ ] Be able to add elements to the render queue via SPI
- [ ] Be able to send a command to read data from the SD card and render it
- [ ] Be able to play audio with a SPI command

## Misc
- [x] USB host mode for keyboard support (either using on-chip USB host or external USB->Serial FTDI)
  - Not part of the library, the user can use TinyUSB by themselves.
- [x] Organized SDK file structure, with one master include header file
- [x] Persistent saves on-chip (is this possible?)
  - Just save to the SD card. It's easier in software and easier for the user.
- [ ] Simple menu-making functions
- [ ] Good documentation

## PCB (Rev 2)
- [x] Add pads to pico pin holes so it can be either mounted with header pins, female header sockets, or surface mounted with the castellated pads
  - [x] Add alternate pads for the resistor DAC on the back (used when the pico is surface mounted)
- [x] External SPI EEPROM chip for save states and persistent memory (option to socket OR solder, so save states can be moved between boards)
  - This is dumb, just save to the SD card. Not doing it.
- [x] Replace controller connectors with I2C leads (see Controller Rev 2 below for more details)
  - [x] Add controller programming headers on PCB, but leave headers unsoldered by default
    - Not doing it, at least right now. This is going to be a nightmare for software development, just grab an arduino and program it that way.
- [ ] Audio chip upgrade -- digital audio?
  - PWM audio sounds surprisingly good, so I'm keeping it for now. It's also way cheaper -- the audio DAC I would use is more expensive, needs a lot of other components (filtering caps), and is more expensive in CPU time.

## PCB - Pico-VGA Mini
- [ ] Simplified board, containing everything but the controller ports. Made to be used in "peripheral mode" via the 4-pin SPI header and another microcontroller rather than the controllers

## Controller (Rev 2)
- [ ] Replace the complex controller logic with an ATTiny24 mounted on the controller
- [ ] Replace connector with:
  - Ethernet jack on both ends
  - Phone/Modem cable (4 conductors)
  - USB-A (2.0) on the pico-VGA side, USB-C (2.0) on controller side
- [ ] Move resistor network to controller for pulldowns
- [ ] Add some kind of indicator to what player you are (leds?)
- [ ] Have holes available to solder a programming port on

# Ideas
- Teensy-VGA, running a Teensy 4.x? Would support higher resolutions, more complex rendering, possibly higher color depth (16 bit)
- Bluetooth chip for Xbox or Playstation controllers? Is this even possible?
  - I think this is possible with pico W -- I think I would leave this up to users, they can get their own inputs and this lib will focus on audio/video output only.