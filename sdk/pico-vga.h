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
typedef struct RenderQueueItem RenderQueueItem;

//RenderQueueItem.type
#define RQI_T_REMOVED          0
#define RQI_T_FILL             1
#define RQI_T_PIXEL            2
#define RQI_T_LINE             3
#define RQI_T_RECTANGLE        4
#define RQI_T_FILLED_RECTANGLE 5
#define RQI_T_TRIANGLE         6
#define RQI_T_FILLED_TRIANGLE  7
#define RQI_T_CIRCLE           8
#define RQI_T_FILLED_CIRCLE    9
#define RQI_T_STR              10
#define RQI_T_SPRITE           11
#define RQI_T_BITMAP           12
#define RQI_T_POLYGON          13
#define RQI_T_FILLED_POLYGON   14
#define RQI_T_LIGHT            15
#define RQI_T_SVG              16

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
} Controller;

extern volatile Controller C1;
extern volatile Controller C2;
extern volatile Controller C3;
extern volatile Controller C4;


/*
        Constants
=========================
*/
#define FRAME_SCALER 2 //Resolution Scaler

#define FRAME_HEIGHT (600/FRAME_SCALER)
#define FRAME_WIDTH (800/FRAME_SCALER)
#define FRAME_FULL_HEIGHT (628/FRAME_SCALER) //The full height/width of the frame, including porches, sync, etc
#define FRAME_FULL_WIDTH (1056/FRAME_SCALER)

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
int initDisplay(bool autoRenderEn);
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

#endif