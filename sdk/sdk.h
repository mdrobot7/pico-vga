//The private/internal header file for the sdk. DO NOT include this, include pico-vga.h instead!

#ifndef SDK_H
#define SDK_H

#include "pico-vga.h"
#include "pinout.h"

extern volatile uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];
extern uint8_t * frameReadAddr[FRAME_FULL_HEIGHT*FRAME_SCALER];
extern uint8_t BLANK[FRAME_WIDTH];

//Configuration Options
extern uint8_t autoRender;
extern uint8_t antiAliasing;

extern volatile RenderQueueItem background; //First element of the linked list, can be reset to any background
extern volatile RenderQueueItem *lastItem; //Last item in linked list, used to set *last in RenderQueueItem

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8
extern uint8_t *font; //The current font in use by the system

inline void busyWait(uint64_t n) {
    for(; n > 0; n--) asm("nop");
}

void initController();

void render();

#endif