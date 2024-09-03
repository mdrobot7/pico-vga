// The public/external header file for the pico-vga SDK. Include this into the main c file.

#ifndef __PICO_VGA_H
#define __PICO_VGA_H

#include "pico/stdlib.h"

#include "audio.h"
#include "color.h"
// #include "controller.h"
#include "draw.h"
#include "font.h"
#include "pinout.h"

#ifndef __packed
#define	__packed	__attribute__((__packed__))
#endif
#ifndef __aligned(x)
#define	__aligned(x)	__attribute__((__aligned__(x)))
#endif

// The maximum size of the frame buffer for the pico-vga renderer. The renderer will use
// either the allocated size or the size of 1 frame, whichever is smaller.
// Allocated at compile-time, so be careful of other memory-intensive parts of your program.
#ifndef PV_FRAMEBUFFER_BYTES
#define PV_FRAMEBUFFER_BYTES 200000
#endif

//Switch to true if running in peripheral mode
#ifndef PV_PERIPHERAL_MODE
#define PV_PERIPHERAL_MODE false
#endif

#endif