// The public/external header file for the pico-vga SDK. Include this into the main c file.

#ifndef __PICO_VGA_H
#define __PICO_VGA_H

#include "audio/audio.h"
#include "pico/stdlib.h"
#include "vga/color.h"
// #include "controller.h"
#include "pinout.h"
#include "vga/font.h"
#include "vga/vga.h"

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif
#ifndef __aligned
#define __aligned(x) __attribute__((__aligned__(x)))
#endif

#endif