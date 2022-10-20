#ifndef SDK_H
#define SDK_H

#include "main.h"
#include "sdk-chars.h"

//This file includes all function headers for the pico-vga SDK.
//The SDK is meant to help developers make games/programs for
//the pico-vga hardware.

/*
    Chart for char type;
'p' : point
'l' : line
'r' : rectangle
't' : triangle
'o' : circle

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

//This is the queue of items to be rendered onto the screen.
//Item 0 will be rendered first, then item 1, etc etc
#define RENDER_QUEUE_LEN 256
RenderQueueItem renderQueue[RENDER_QUEUE_LEN];

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

Controller controller;

void initController(); //zeroes out Controller struct
void setController(Controller *c);


//Basic Drawing
//xN and yN are coordinates, in pixels, color is the color of the element.
//draw functions just draw the element, fill elements draw and fill it in.

void drawPixel(uint16_t x, uint16_t y, uint8_t color);
void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
void drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
void drawNPoints(uint16_t points[][2], uint8_t len, uint8_t color); //draws a path between all points in the list

void fillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t fill);
void fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t fill);
void fillCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color, uint8_t fill);

void fillScreen(uint8_t color);
void clearScreen();


//Advanced drawing

//Draws the chars from the default character library. Dimensions are 5x8 pixels each.
void drawText(uint16_t x, uint16_t y, char *str, uint8_t color, uint8_t scale);

//Text helper functions:
void setTextFont(uint8_t newFont[][8]);

void drawSprite(uint8_t *sprite, uint16_t dimX, uint16_t dimY);

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