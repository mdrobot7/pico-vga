#ifndef PINOUT_H
#define PINOUT_H

/*
        PINOUT (OLD)
GPIO0 - Color MSB (RED)
GPIO 1-6 - Color
GPIO7 - Color LSB (BLUE)
GPIO8 - HSync
GPIO9 - VSync
*/

/*
        PINOUT (NEW)
GPIO0 - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7 - Color MSB (RED 1)
GPIO8 - HSync
GPIO9 - VSync
GPIO10 - DPad Up
GPIO11 - DPad Down
GPIO12 - DPad Left
GPIO13 - DPad Right
GPIO14 - A
GPIO15 - B
GPIO16 - SD Card MISO (RX)
GPIO17 - SD Card Chip Select
GPIO18 - SD Card CLK
GPIO19 - SD Card MOSI (TX)
GPIO20 - X
GPIO21 - Y
GPIO22 - Controller Select A

GPIO26 - Controller Select B
GPIO27 - Audio L
GPIO28 - Audio R
*/

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

#endif