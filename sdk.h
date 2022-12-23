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
'h' : hidden (keep it in the render queue, but don't display it)
'n' : removed (this slot in the render queue is now open)

*/

//Linked list for the queue of items to be rendered to the screen
struct RenderQueueItem {
    struct RenderQueueItem *next;
    char type;
    uint8_t update; //true if this item should be updated on the next render pass, false otherwise
    
    uint16_t x1; //rectangular bounding box for the item
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

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
extern uint8_t frameScaler;

#define FRAME_HEIGHT (600/frameScaler)
#define FRAME_WIDTH (800/frameScaler)
#define FRAME_FULL_HEIGHT (628/frameScaler) //The full height/width of the frame, including porches, sync, etc
#define FRAME_FULL_WIDTH (1056/frameScaler)

#define COLOR_BLACK 0b00000000 
#define COLOR_RED   0b11100000
#define COLOR_GREEN 0b00011100
#define COLOR_BLUE  0b00000011
//Fill in rest of basic colors here


/*
        Functions
=========================
*/
void initSDK(Controller *c);
void updateDisplay();

extern volatile RenderQueueItem background;

//Basic Drawing
//xN and yN are coordinates, in pixels, color is the color of the element.
//draw functions just draw the element, fill elements draw and fill it in.
//All functions return the index in the render queue that the element takes up.

void setBackground(uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color);

RenderQueueItem * drawPixel(RenderQueueItem* prev, uint16_t x, uint16_t y, uint8_t color);
RenderQueueItem * drawLine(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueItem * drawRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueItem * drawTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
RenderQueueItem * drawCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
RenderQueueItem * drawNPoints(RenderQueueItem* prev, uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

RenderQueueItem * fillRectangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
RenderQueueItem * fillTriangle(RenderQueueItem* prev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
RenderQueueItem * fillCircle(RenderQueueItem* prev, uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

RenderQueueItem * fillScreen(RenderQueueItem* prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color);
void clearScreen();

RenderQueueItem * drawSprite(RenderQueueItem* prev, uint8_t *sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, uint8_t scale);

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
RenderQueueItem * drawText(RenderQueueItem* prev, uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale);

//Text helper functions:
void setTextFont(uint8_t *newFont);

//Utilities:
uint8_t rgbToByte(uint8_t r, uint8_t g, uint8_t b);
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