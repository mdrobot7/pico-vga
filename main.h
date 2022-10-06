#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "color.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

extern uint8_t frame[2][FRAME_HEIGHT][FRAME_WIDTH]; //2 frames, 240 rows by 320 columns. 76.8KB per frame, 153.6KB for the entire array.
extern uint8_t activeFrame; //the frame currently being printed
#endif