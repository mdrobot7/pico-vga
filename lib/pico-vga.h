//The public/external header file for the pico-vga SDK. Include this into the main c file.

#ifndef PICO_VGA_H
#define PICO_VGA_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

//The *total* amount of memory that the entire Pico-VGA library is allowed to use (framebuffer, render elements, etc).
//Try to maximize this, since the more memory the library is given the better it will perform (recommended: 256kB)
//Allocated at compile-time, so be careful of other memory-intensive parts of your program.
#define PICO_VGA_MAX_MEMORY_BYTES 200000

/*
        Typedefs
========================
*/

//The Unique ID of an item in the render queue, used to modify that item after initialization.
typedef uint32_t RenderQueueUID_t;


/*
        Module Configuration Structs
============================================
*/
//Switch to true if running in peripheral mode
#define PERIPHERAL_MODE false

extern uint16_t frameWidth;
extern uint16_t frameHeight;
extern uint16_t frameFullWidth;
extern uint16_t frameFullHeight;

typedef enum {
    RES_800x600,
    RES_640x480,
    RES_1024x768,
} DisplayConfigBaseResolution_t;

typedef enum {
    RES_SCALED_800x600 = 1,
    RES_SCALED_400x300 = 2,
    RES_SCALED_200x150 = 4,
    RES_SCALED_100x75 = 8,
    RES_SCALED_640x480 = 1,
    RES_SCALED_320x240 = 2,
    RES_SCALED_160x120 = 4,
    RES_SCALED_80x60 = 8,
    RES_SCALED_1024x768 = 1,
} DisplayConfigResolutionScale_t;

typedef struct {
    PIO pio; //Which PIO to use for color
    DisplayConfigBaseResolution_t baseResolution;
    DisplayConfigResolutionScale_t resolutionScale;
    bool autoRender; //Turn on autoRendering (no manual updateDisplay() call required)
    bool antiAliasing; //Turn antialiasing on or off
    uint32_t frameBufferSizeBytes; //Cap the size of the frame buffer (if unused, set to 0xFFFF) -- Initializer will write back the amount of memory used. Default: Either the size of one frame or 75% of PICO_VGA_MAX_MEMORY_BYTES, whichever is less.
    uint8_t numInterpolatedLines; //Override the default number of interpolated frame lines -- used if the frame buffer is not large enough to hold all of the frame data. Default = 2.
    uint32_t renderQueueSizeBytes; //Cap the size of the render queue (if unused, set to 0).
    bool peripheralMode; //Enable command input via SPI.
    bool clearRenderQueueOnDeInit; //Clear the render queue after a call to deInitPicoVGA()
    bool disconnectDisplay; //Setting to true causes the display to disconnect and go to sleep after a call to deInitPicoVGA().
    bool interpolatedRenderingMode; //Set the rendering mode when interpolating lines. False: begin interpolating lines after rendering the rest of the frame. True: Interpolate lines as they are displayed in real time
    uint16_t colorDelayCycles;
} DisplayConfig_t;

#define MAX_NUM_CONTROLLERS 4
typedef struct __packed {
    uint8_t a;
} ControllerConfig_t;

typedef struct {
    bool stereo; //False = mono audio, true = stereo
} AudioConfig_t;

typedef struct {
    uint8_t a;
} SDConfig_t;

typedef struct {
    uint8_t a;
} PeripheralModeConfig_t;


/*
        Initialization
==============================
*/
int initPicoVGA(DisplayConfig_t * displayConf, ControllerConfig_t * controllerConf, AudioConfig_t * audioConf, SDConfig_t * sdConf);
int deInitPicoVGA(bool closeDisplay, bool closeController, bool closeAudio, bool closeSD);
void updateFullDisplay();


/*
        Drawing
=======================
*/
RenderQueueUID_t drawPixel(uint16_t x, uint16_t y, uint8_t color);
RenderQueueUID_t drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
RenderQueueUID_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t thickness, uint8_t color);

RenderQueueUID_t drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueUID_t drawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
RenderQueueUID_t drawFilledCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
RenderQueueUID_t fillPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t color);
RenderQueueUID_t fillScreen(uint8_t * obj, uint8_t color);

RenderQueueUID_t drawText(uint16_t x1, uint16_t y, uint16_t x2, char *str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverrideBytes);
void setTextFont(uint8_t *newFont);

RenderQueueUID_t drawSprite(uint8_t * sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, int8_t scaleX, int8_t scaleY);

void clearScreen();


/*
        Drawing Modifiers
=================================
*/
void setHidden(RenderQueueUID_t itemUID, bool hidden);
void removeItem(RenderQueueUID_t itemUID);


/*
        Controllers
===========================
*/
bool controller_pair();
void controller_unpair(uint8_t controller);
void controller_unpair_all();
uint8_t controller_get_num_paired_controllers();

typedef enum {
    CONTROLLER_BUTTON_UP,
    CONTROLLER_BUTTON_DOWN,
    CONTROLLER_BUTTON_LEFT,
    CONTROLLER_BUTTON_RIGHT,
    CONTROLLER_BUTTON_A,
    CONTROLLER_BUTTON_B,
    CONTROLLER_BUTTON_X,
    CONTROLLER_BUTTON_Y,
} controller_button_t;

bool controller_get_button_state(uint8_t controller, controller_button_t button);


/*
        Utilities
=========================
*/

/**
 * @brief Convert a 3 byte HTML color code into an 8 bit color.
 * 
 * @param color The HTML color code.
 * @return An 8 bit compressed color.
 */
uint8_t HTMLTo8Bit(uint32_t color);

/**
 * @brief Convert red, green, and blue color values into a single 8 bit color.
 * 
 * @param r Red value
 * @param g Blue value
 * @param b Green value
 * @return An 8 bit compressed color.
 */
uint8_t rgbTo8Bit(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Convert HSV colors into 8 bit compressed RGB.
 * @param hue Hue value, 0-255
 * @param saturation Saturation value, 0-255
 * @param value Brightness, 0-255
 * @return An 8 bit compressed color 
 */
uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value);

/**
 * @brief Invert a color.
 * 
 * @param color A color to invert.
 * @return The inverted version of the supplied color.
 */
uint8_t invertColor(uint8_t color);

/*
        Constants
=========================
*/
#define COLOR_WHITE   0b11111111
#define COLOR_SILVER  0b10110110
#define COLOR_GRAY    0b10010010
#define COLOR_BLACK   0b00000000

#define COLOR_RED     0b11100000
#define COLOR_MAROON  0b10000000
#define COLOR_YELLOW  0b11111100
#define COLOR_OLIVE   0b10010000
#define COLOR_LIME    0b00011100
#define COLOR_GREEN   0b00010000
#define COLOR_TEAL    0b00010010
#define COLOR_CYAN    0b00011111
#define COLOR_BLUE    0b00000011
#define COLOR_NAVY    0b00000010
#define COLOR_MAGENTA 0b11100011
#define COLOR_PURPLE  0b10000010


/*
        Default Configuration Structs
=============================================
*/

//Default configuration for DisplayConfig_t
#define DISPLAY_CONFIG_DEFAULT {\
    .PIO = pio0\
    .baseResolution = RES_800x600,\
    .resolutionScale = RES_SCALED_400x300,\
    .autoRender = true,\
    .antiAliasing = false,\
    .frameBufferSizeBytes = 0xFFFFFFFF,\
    .renderQueueSizeBytes = 0,\
    .numInterpolatedLines = 0,\
    .peripheralMode = false,\
    .clearRenderQueueOnDeInit = false,\
    .disconnectDisplay = false,\
    .interpolatedRenderingMode = false,\
    .colorDelayCycles = 0,\
}

//Default configuration for ControllerConfig_t
#define CONTROLLER_CONFIG_DEFAULT {\
    .a = 0,\
}

//Default configuration for AudioConfig_t
#define AUDIO_CONFIG_DEFAULT {\
    .stereo = true,\
}

//Default configuration for SDConfig_t
#define SD_CONFIG_DEFAULT {\
    .a = 0,\
}

//Default configuration for PeripheralModeConfig_t
#define PERIPHERAL_MODE_CONFIG_DEFAULT {\
    .a = 0,\
}

#endif