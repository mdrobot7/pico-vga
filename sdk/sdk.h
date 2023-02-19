//The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef SDK_H
#define SDK_H

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

const uint16_t frameSize[3][4] = {{800, 600, 1056, 628},
                                  {640, 480, 800, 525},
                                  {1024, 768, 1344, 806}};

extern volatile uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];
extern uint8_t * frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER];
extern uint8_t BLANK[FRAME_WIDTH];

//Configuration Options
extern DisplayConfig_t * displayConfig;
extern ControllerConfig_t * controllerConfig;
extern AudioConfig_t * audioConfig;
extern SDConfig_t * sdConfig;
extern USBHostConfig_t * usbConfig;

extern volatile RenderQueueItem background; //First element of the linked list, can be reset to any background
extern volatile RenderQueueItem *lastItem; //Last item in linked list, used to set *last in RenderQueueItem

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8
extern uint8_t *font; //The current font in use by the system

inline void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

void initDisplay();
void initController();
void initAudio();
void initSD();
void initUSB();

void render();

#endif