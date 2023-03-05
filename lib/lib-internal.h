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
#include "vsync_640x480.pio.h"
#include "hsync_640x480.pio.h"
#include "vsync_800x600.pio.h"
#include "hsync_800x600.pio.h"
#include "vsync_1024x768.pio.h"
#include "hsync_1024x768.pio.h"

extern const uint16_t frameSize[3][4];

//Unified Pico-VGA memory buffer
extern uint8_t buffer[PICO_VGA_MAX_MEMORY_BYTES];

/* Pointers to each block in the Pico VGA unified memory buffer. Written in order of how it's organized
 * (render queue first, at buffer[0]). End pointers are where the next element of the render queue, for example,
 * will be added, *not* where the block ends (it ends at the start of the next one).
*/
extern uint8_t * renderQueueStart;
extern uint8_t * renderQueueEnd;
extern uint8_t * sdStart;
extern uint8_t * sdEnd;
extern uint8_t * audioStart;
extern uint8_t * audioEnd;
extern uint8_t * controllerStart;
extern uint8_t * controllerEnd;
extern uint8_t * frameReadAddrStart;
extern uint8_t * frameReadAddrEnd;
extern uint8_t * blankLineStart;
extern uint8_t * interpolatedLineStart;
extern uint8_t * frameBufferStart;
extern uint8_t * frameBufferEnd;

extern uint8_t frameCtrlDMA;
extern uint8_t frameDataDMA;
extern uint8_t blankDataDMA;
extern uint8_t controllerDataDMA;

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
int initPeripheralMode();

int deInitDisplay();
int deInitController();
int deInitAudio();
int deInitSD();
int deInitUSB();
int deInitPeripheralMode();

void render();

#endif