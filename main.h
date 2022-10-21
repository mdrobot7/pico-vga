#ifndef MAIN_H
#define MAIN_H

#include "sdk.h"

#include "color.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"
#include "colorHandler.pio.h"

#define FRAME_WIDTH 400
#define FRAME_HEIGHT 300
extern uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH]; //1 frame, 300 rows by 400 columns. 120kB array.
#endif