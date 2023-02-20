//The public/external header file for the pico-vga SDK. Include this into the main c file.

#ifndef PICO_VGA_H
#define PICO_VGA_H

#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*
        Render Queue
============================
*/
//RenderQueueItem_t.type
typedef enum {
    RQI_T_REMOVED,
    RQI_T_FILL,
    RQI_T_PIXEL,
    RQI_T_LINE,
    RQI_T_RECTANGLE,
    RQI_T_FILLED_RECTANGLE,
    RQI_T_TRIANGLE,
    RQI_T_FILLED_TRIANGLE,
    RQI_T_CIRCLE,
    RQI_T_FILLED_CIRCLE,
    RQI_T_STRING,
    RQI_T_SPRITE,
    RQI_T_BITMAP,
    RQI_T_POLYGON, 
    RQI_T_FILLED_POLYGON,
    RQI_T_LIGHT,
    RQI_T_SVG,
} RenderQueueItemType_t;

//Linked list item for the queue of items to be rendered to the screen
struct RenderQueueItem_t {
    struct RenderQueueItem_t * next;
    RenderQueueItemType_t type;

    uint8_t flags; //Bit register for parameters
    
    uint16_t x1; //rectangular bounding box for the item
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    uint8_t scale;
    float rotation;

    uint8_t color;
    uint8_t * obj;
};
typedef struct RenderQueueItem_t RenderQueueItem_t;

//RenderQueueItem.flags macros
#define RQI_UPDATE   (1u << 0)
#define RQI_HIDDEN   (1u << 1)
#define RQI_WORDWRAP (1u << 2)


/*
        Controllers
===========================
*/
typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t u;
    uint8_t d;
    uint8_t l;
    uint8_t r;
} Ctrler_t;

typedef struct {
    uint8_t maxNumControllers;
    Ctrler_t * p;
} Controllers_t;


/*
        Module Configuration Structs
============================================
*/
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
    DisplayConfigBaseResolution_t baseResolution;
    DisplayConfigResolutionScale_t resolutionScale;
    uint8_t autoRender; //Turn on autoRendering (no manual updateDisplay() call required)
    uint8_t antiAliasing;
    uint16_t frameBufferSizeKB; //Cap the size of the frame buffer (if unused, set to 0xFFFF)
    uint8_t numInterpolatedLines; //Override the default number of interpolated frame lines -- used if the frame buffer is not large enough to hold all of the frame data. Default = 2.
    uint8_t peripheralMode; //Enable command input via SPI.
    uint8_t clearRenderQueueOnDeInit;
    uint16_t colorDelayCycles;
} DisplayConfig_t;

typedef struct {
    uint8_t maxNumControllers;
    Controllers_t * controllers;
} ControllerConfig_t;

typedef struct {
    uint8_t a;
} AudioConfig_t;

typedef struct {
    uint8_t a;
} SDConfig_t;

typedef struct {
    uint8_t a;
} USBHostConfig_t;


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
        Functions
=========================
*/
int initPicoVGA(DisplayConfig_t * displayConf, ControllerConfig_t * controllerConf, AudioConfig_t * audioConf, SDConfig_t * sdConf, USBHostConfig_t * usbConf);
int deInitPicoVGA(bool closeDisplay, bool closeController, bool closeAudio, bool closeSD, bool closeUSB);
void updateFullDisplay();

extern volatile RenderQueueItem_t background;

//Basic Drawing
//xN and yN are coordinates, in pixels, color is the color of the element.
//draw functions just draw the element, fill elements draw and fill it in.
//All functions return the index in the render queue that the element takes up.

RenderQueueItem_t * drawPixel(RenderQueueItem_t * prev, uint16_t x, uint16_t y, uint8_t color);
RenderQueueItem_t * drawLine(RenderQueueItem_t * prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
RenderQueueItem_t * drawRectangle(RenderQueueItem_t * prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueItem_t * drawTriangle(RenderQueueItem_t * prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
RenderQueueItem_t * drawCircle(RenderQueueItem_t * prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
RenderQueueItem_t * drawPolygon(RenderQueueItem_t * prev, uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

RenderQueueItem_t * drawFilledRectangle(RenderQueueItem_t * prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
RenderQueueItem_t * drawFilledTriangle(RenderQueueItem_t * prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
RenderQueueItem_t * drawFilledCircle(RenderQueueItem_t * prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

RenderQueueItem_t * fillScreen(RenderQueueItem_t * prev, uint8_t * obj, uint8_t color);
void clearScreen();

RenderQueueItem_t * drawSprite(RenderQueueItem_t * prev, uint8_t *sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, uint8_t scale);

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
RenderQueueItem_t * drawText(RenderQueueItem_t * prev, uint16_t x1, uint16_t y, uint16_t x2, char *str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverride);
void setTextFont(uint8_t *newFont);

//Modifiers:
void setBackground(uint8_t * obj, uint8_t color);
void setHidden(RenderQueueItem_t * item, uint8_t hidden);
void setColor(RenderQueueItem_t * item, uint8_t color);
void setCoordinates(RenderQueueItem_t * item, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void removeItem(RenderQueueItem_t * item);

//Utilities:
uint8_t HTMLTo8Bit(uint32_t color);
uint8_t rgbTo8Bit(uint8_t r, uint8_t g, uint8_t b);
uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value);
uint8_t invertColor(uint8_t color);

#endif