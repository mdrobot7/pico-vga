#ifndef SDK_H
#define SDK_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "pico/malloc.h"

#include "color.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"

#include "pinout.h"

//This file includes all function headers and dependencies for the pico-vga SDK.
//The SDK is meant to help developers make games/programs for the pico-vga hardware.


/*
        Structs
=======================
*/
/*
    Chart for char type;
'p' : pixel
'l' : line
'r' : rectangle
't' : triangle
'o' : circle

'R' : filled rectangle
'T' : filled triangle
'O' : filled circle

'f' : fill entire screen

'c' : char/string
's' : sprite

Special values:
'n' : removed (this slot in the render queue is now open)

*/

//Linked list for the queue of items to be rendered to the screen
struct RenderQueueItem {
    struct RenderQueueItem *next;
    char type;

    uint8_t flags; //Bit register for parameters
    /*
    Bit |   Purpose
    0   | Update -- set to 1 to update this item when in autoRender mode
    1   | Hidden -- set to 1 to hide this item
    2   | 
    3   | 
    4   | 
    5   | 
    6   | 
    7   | 
    */
    
    uint16_t x1; //rectangular bounding box for the item
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    uint8_t thickness;

    uint8_t color;
    uint8_t *obj;
};

typedef struct RenderQueueItem RenderQueueItem;

//Struct for one controller
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
int initDisplay(Controller *P1, Controller *P2, Controller *P3, Controller *P4, uint8_t autoRenderEn);
void updateDisplay();

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
RenderQueueItem * drawNPoints(RenderQueueItem* prev, uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

RenderQueueItem * drawFilledRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
RenderQueueItem * drawFilledTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
RenderQueueItem * drawFilledCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

RenderQueueItem * fillScreen(RenderQueueItem* prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color);
void clearScreen();

RenderQueueItem * drawSprite(RenderQueueItem* prev, uint8_t *sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, uint8_t scale);

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
RenderQueueItem * drawText(RenderQueueItem* prev, uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale);
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

/*
Draw text algorithm:
params: x, y, string (char *), color, scale
character data:
- each character is stored as an array of BITS (NOT bytes) to save space
- it is stored in a massive look up table, basically an ascii chart (based off of IBM CP437 character chart)

algorithm:
- loop through string/char array UNTIL it reaches '\0' null character (see learn-c)
- cast each char as an int
- look it up in look up table/array
- print it out
- repeat
*/

#endif