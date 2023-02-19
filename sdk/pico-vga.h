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
//Linked list item for the queue of items to be rendered to the screen
struct RenderQueueItem {
    struct RenderQueueItem *next;
    uint8_t type;

    uint8_t flags; //Bit register for parameters
    
    uint16_t x1; //rectangular bounding box for the item
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    uint8_t scale;
    float rotation;

    uint8_t color;
    uint8_t *obj;
};
typedef struct RenderQueueItem RenderQueueItem_t;

//RenderQueueItem.type
typedef enum {
    REMOVED,
    FILL,
    PIXEL,
    LINE,
    RECTANGLE,
    FILLED_RECTANGLE,
    TRIANGLE,
    FILLED_TRIANGLE,
    CIRCLE,
    FILLED_CIRCLE,
    STRING,
    SPRITE,
    BITMAP,
    POLYGON, 
    FILLED_POLYGON,
    LIGHT,
    SVG,
} RenderQueueItemType;

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
    uint8_t maxNumControllers,
    Ctrler_t *p,
} Controllers_t;


/*
        Module Configuration Structs
============================================
*/
typedef enum {
    RES_800x600,
    RES_640x480,
    RES_1024x768,
} DisplayConfigBaseResolution_t;

typedef enum {
    RES_SCALED_800x600 = 1,
    RES_SCALED_400x300,
    RES_SCALED_200x150,
    RES_SCALED_100x75,
    RES_SCALED_640x480 = 1,
    RES_SCALED_320x240,
    RES_SCALED_160x120,
    RES_SCALED_80x60,
    RES_SCALED_1024x768 = 1,
} DisplayConfigResolutionScale_t;

typedef struct {
    DisplayConfigBaseResolution_t baseResolution,
    DisplayConfigResolutionScale_t resolutionScale,
    uint8_t autoRender,
    uint8_t antiAliasing,
    uint32_t memoryAllocatedKB,
    uint8_t peripheralMode,
} DisplayConfig_t;

typedef struct {
    uint8_t maxNumControllers,
    Controllers_t * controllers,
} ControllerConfig_t;

typedef struct {
    uint8_t a,
} AudioConfig_t;

typedef struct {
    uint8_t a,
} SDConfig_t;

typedef struct {
    uint8_t a,
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
int initPicoVGA(DisplayConfig_t * displayConf, ControllerConfig_t * controllerConf, AudioConfig_t * audioConf, SDConfig_t * sdConf, USBHostConfig_t * usbConf)
void updateFullDisplay();

extern volatile RenderQueueItem background;

//Basic Drawing
//xN and yN are coordinates, in pixels, color is the color of the element.
//draw functions just draw the element, fill elements draw and fill it in.
//All functions return the index in the render queue that the element takes up.

RenderQueueItem * drawPixel(RenderQueueItem* prev, uint16_t x, uint16_t y, uint8_t color);
RenderQueueItem * drawLine(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
RenderQueueItem * drawRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueItem * drawTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
RenderQueueItem * drawCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
RenderQueueItem * drawPolygon(RenderQueueItem* prev, uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

RenderQueueItem * drawFilledRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
RenderQueueItem * drawFilledTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
RenderQueueItem * drawFilledCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

RenderQueueItem * fillScreen(RenderQueueItem* prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color);
void clearScreen();

RenderQueueItem * drawSprite(RenderQueueItem* prev, uint8_t *sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, uint8_t scale);

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
RenderQueueItem * drawText(RenderQueueItem *prev, uint16_t x1, uint16_t y, uint16_t x2, char *str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverride);
void setTextFont(uint8_t *newFont);

//Modifiers:
void setBackground(uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color);
void setHidden(RenderQueueItem *item, uint8_t hidden);
void setColor(RenderQueueItem *item, uint8_t color);
void setCoordinates(RenderQueueItem *item, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void removeItem(RenderQueueItem *item);

//Utilities:
uint8_t HTMLTo8Bit(uint32_t color);
uint8_t rgbTo8Bit(uint8_t r, uint8_t g, uint8_t b);
uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value);
uint8_t invertColor(uint8_t color);

uint16_t getFrameWidth();
uint16_t getFrameHeight();
uint16_t getFrameFullWidth();
uint16_t getFrameFullHeight();

#endif