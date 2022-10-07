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
#include "colorHandler.pio.h"

#define FRAME_WIDTH 400
#define FRAME_HEIGHT 300

extern uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH]; //1 frame, 300 rows by 400 columns. 120kB array.
#endif