//The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef SDK_H
#define SDK_H

#include "pico-vga.h"
#include "pinout.h"

volatile uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];
uint8_t * frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER];
uint8_t BLANK[FRAME_WIDTH];

//Configuration Options
uint8_t autoRender = 0;

volatile RenderQueueItem background = { //First element of the linked list, can be reset to any background
    .type = 'f',
    .color = 0,
    .obj = NULL,
    .next = NULL,
    .flags = RQI_UPDATE
};
volatile RenderQueueItem *lastItem = &background; //Last item in linked list, used to set *last in RenderQueueItem

inline void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

void initController();

void render();

#endif