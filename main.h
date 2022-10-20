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

//Pin Definitions
#define HSYNC_PIN 8
#define VSYNC_PIN 9
#define UP_PIN 10
#define DOWN_PIN 11
#define LEFT_PIN 12
#define RIGHT_PIN 13
#define A_PIN 14
#define B_PIN 15
#define X_PIN 20
#define Y_PIN 21
#define CONTROLLER_SEL_A_PIN 22
#define CONTROLLER_SEL_B_PIN 26

#define AUDIO_L_PIN 27
#define AUDIO_R_PIN 28

#define SD_MOSI_PIN 19
#define SD_MISO_PIN 16
#define SD_CLK_PIN 18
#define SD_CS_PIN 17


#define FRAME_WIDTH 400
#define FRAME_HEIGHT 300

extern uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH]; //1 frame, 300 rows by 400 columns. 120kB array.
#endif