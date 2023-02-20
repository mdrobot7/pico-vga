//The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef LIB_INTERNAL_H
#define LIB_INTERNAL_H

#include "pico-vga.h"
#include "pinout.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

#include "color.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"

extern const uint16_t frameSize[3][4];

extern volatile uint8_t * frame;
extern uint8_t ** frameReadAddr;
extern uint8_t * BLANK;
extern uint8_t * line;

//Configuration Options
extern DisplayConfig_t * displayConfig;
extern ControllerConfig_t * controllerConfig;
extern AudioConfig_t * audioConfig;
extern SDConfig_t * sdConfig;
extern USBHostConfig_t * usbConfig;

extern volatile RenderQueueItem_t  background; //First element of the linked list, can be reset to any background
extern volatile RenderQueueItem_t * lastItem; //Last item in linked list, used to set *last in RenderQueueItem

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8
extern uint8_t *font; //The current font in use by the system

inline void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

int initDisplay();
int initController();
int initAudio();
int initSD();
int initUSB();

void render();

#endif