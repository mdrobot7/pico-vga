#ifndef SDK_H
#define SDK_H

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"

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

typedef struct {
    char type;
    uint16_t x;
    uint16_t y;
    uint16_t h;
    uint16_t w;
    uint8_t color;
    uint8_t *obj;
} RenderQueueItem;

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
        Functions
=========================
*/
#define FRAME_WIDTH 400
#define FRAME_HEIGHT 300

void initPIO();
void initSDK(Controller *c);

//This is the queue of items to be rendered onto the screen.
//Item 0 will be rendered first, then item 1, etc etc
#define RENDER_QUEUE_LEN 256
extern RenderQueueItem renderQueue[RENDER_QUEUE_LEN];

extern Controller controller;


//Basic Drawing
//xN and yN are coordinates, in pixels, color is the color of the element.
//draw functions just draw the element, fill elements draw and fill it in.
//All functions return the index in the render queue that the element takes up.

int16_t drawPixel(uint16_t x, uint16_t y, uint8_t color);
int16_t drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
int16_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
int16_t drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
int16_t drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
int16_t drawNPoints(uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

int16_t fillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
int16_t fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
int16_t fillCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

int16_t fillScreen(uint8_t color, bool clearRenderQueue);
void clearScreen();


//Advanced drawing

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
int16_t drawText(uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale);

//Text helper functions:
void setTextFont(uint8_t *newFont);

int16_t drawSprite(uint16_t x, uint16_t y, uint8_t *sprite, uint16_t dimX, uint16_t dimY, uint8_t colorOverride, uint8_t scale);

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