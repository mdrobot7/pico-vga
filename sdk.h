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
    uint16_t x1; //rectangular bounding box for the item
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint8_t color;
    uint8_t *obj;
};

typedef struct RenderQueueItem RenderQueueItem;

//The controller button struct -- read this to get controller button values
typedef struct {
    struct Controller1 {
        uint8_t a;
        uint8_t b;
        uint8_t x;
        uint8_t y;
        uint8_t u;
        uint8_t d;
        uint8_t l;
        uint8_t r;
    } C1;
    struct Controller2 {
        uint8_t a;
        uint8_t b;
        uint8_t x;
        uint8_t y;
        uint8_t u;
        uint8_t d;
        uint8_t l;
        uint8_t r;
    } C2;
    struct Controller3 {
        uint8_t a;
        uint8_t b;
        uint8_t x;
        uint8_t y;
        uint8_t u;
        uint8_t d;
        uint8_t l;
        uint8_t r;
    } C3;
    struct Controller4 {
        uint8_t a;
        uint8_t b;
        uint8_t x;
        uint8_t y;
        uint8_t u;
        uint8_t d;
        uint8_t l;
        uint8_t r;
    } C4;
} Controller;


/*
        Constants
=========================
*/
#define FRAME_SCALER 2 //divider for frame width/height, divides resolution by this value for better perf

#define FRAME_HEIGHT (600/FRAME_SCALER)
#define FRAME_WIDTH (800/FRAME_SCALER)
#define FRAME_FULL_HEIGHT (628/FRAME_SCALER) //The full height/width of the frame, including porches, sync, etc
#define FRAME_FULL_WIDTH (1056/FRAME_SCALER)

#define COLOR_NULL  0b00000000
#define COLOR_BLACK 0b00100000 //Black is defined as slightly red, since the sprite code needs a NULL char (above)
                               //to signify "Don't change this pixel" rather than "set it to black"
                               //ALT: have the sprite array, and then have a bit array to say whether or not to display stuff

#define COLOR_RED   0b11100000
#define COLOR_GREEN 0b00011100
#define COLOR_BLUE  0b00000011
//Fill in rest of basic colors here


/*
        Functions
=========================
*/
void initSDK(Controller *c);

extern volatile Controller controller;
extern volatile RenderQueueItem background;

void setRendererState(uint8_t state);

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

RenderQueueItem * fillScreen(RenderQueueItem* prev, uint8_t obj[FRAME_HEIGHT][FRAME_WIDTH], uint8_t color, bool clearRenderQueue);
void clearScreen();


//Advanced drawing

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
RenderQueueItem * drawText(RenderQueueItem* prev, uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale);

//Text helper functions:
void setTextFont(uint8_t *newFont);

RenderQueueItem * drawSprite(RenderQueueItem* prev, uint16_t x, uint16_t y, uint8_t *sprite, uint16_t dimX, uint16_t dimY, uint8_t colorOverride, uint8_t scale);

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